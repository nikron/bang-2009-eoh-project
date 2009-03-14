/**
 * \file fractal-module.c
 * \date March 12, 2008
 *
 * \brief
 * A module to compute a series of frames of zooming in Mandelbrot fractals.
 */

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<gtk/gtk.h>
#include"../base/bang-module-api.h"

#define JOB_FINISHED -1
#define JOB_FREE -2
#define HAS_NO_FRAME -1

char BANG_module_name[8] = "fractal";
unsigned char BANG_module_version[] = {0,0,1};
BANG_api *api;

/* Backing pixmap for drawing area */
static GdkPixmap *pixmap = NULL;
static GtkWidget *window;
static GtkWidget *label;
GtkWidget *drawing_area;
GtkWidget *vbox;
GtkWidget *button;

typedef struct _peerContainer{
	int frame;
	int needsState;
} peerContainer;

typedef struct {
	int x;
	int y;
	int winX;
	int winY;
} fractalState;

typedef struct {
	int frame;
	int x;
	int y;
	int color;
} pixelStruct;

static void job_done_callback(BANG_module_info* mod, BANG_job* job);
static void incoming_job_callback(BANG_module_info* mod, BANG_job* job);
static void jobs_available_callback(BANG_module_info* mod, int authority);
static void outgoing_job_callback(BANG_module_info* mod, int peer);
static void peer_removed_callback(BANG_module_info* mod, int peer);
static pixelStruct* process_frames(int numFrames, int *frame, int *numPixels);
static int mapIterToColor(int iter, int maxIter);

static fractalState fractalStateData = { 0, 0, 0, 0 };
static int totalJobs = 0;
static int jobsLeft = -1;
static int peerArrSize = 0;
static int *jobArr = NULL;
static peerContainer **peerArr = NULL;
int currentFrame = 0;
pixelStruct **frameData;
static int initFlag = 1;

/**
 * Callback for when a finished job is sent to you.
 * \param BANG_job* The job that has been finished.
 */
static void job_done_callback(BANG_module_info* info, BANG_job* job) {
	buffer(job);
	//Decommission job
	jobArr[job->job_number] = -1;
	--jobsLeft;

	if (jobsLeft)
		api->BANG_assert_authority_to_peer(info, job->authority, job->peer);

}

/**
 * Callback for when the authority sends a job to you.
 * \param BANG_job* the job that is sent to you.
 */
static void incoming_job_callback(BANG_module_info* info, BANG_job* job) {
	//Packet: [1B state flag][4B x][4B y][4B frames]*

	int offset = sizeof(char); //Pretend it's already looked at the first byte

	//Read state flag
	//If authority is pushing state data on us
	if (((char*)job->data)[0] == 1) {
		//Read x/y coords
		fractalStateData.x = ((int *)((job->data)+1))[0];
		fractalStateData.y = ((int *)((job->data)+1))[1];
		fractalStateData.winX = ((int *)((job->data)+1))[2];
		fractalStateData.winY = ((int *)((job->data)+1))[3];
		offset += 4*sizeof(int);
	}

	//Read number of frames
	int numFrames = (job->length - offset)/sizeof(int);
	offset += sizeof(char);

	//Read frames
	int *frame = (int *)malloc(numFrames*sizeof(int));
	int i;
	for (i=0; i<numFrames; ++i) {
		frame[i] = ((int *)((job->data)+offset))[0];
		offset += sizeof(int);
	}

	free(job->data);
	int numPixels;
	job->data = (void *)process_frames(numFrames, frame, &numPixels);
	job->length = numPixels;
	api->BANG_finished_request(info, job);
}

/**
 * Callback for when jobs are available for you to request
 * \param int The id of the peer with jobs available.
 */
static void jobs_available_callback(BANG_module_info* info, int authority) {
	api->BANG_request_job(info, authority);
}

static void outgoing_job_callback(BANG_module_info* info, int peer) {
	if (peerArr == NULL || peerArr[peer] == NULL || jobsLeft == 0)
		return;

	int i, nextJob = 0;

	/* If next job has been finished or is being worked on */
	if (jobArr[nextJob] != -1) {
		/* Search for next available job */
		for (i = nextJob+1; jobArr[i] != 0; ++i) {
			if (i == nextJob)
				/* No more active jobs, don't do anything */
				return;
			if (i+1 >= totalJobs)
				/* Reset i to the beginning of the jobArr */
				i = -1;
		}
		nextJob = i;
	}

	//Sign job with peer's ID
	jobArr[nextJob] = peer;
	peerArr[peer]->frame = nextJob;

	BANG_job *job = malloc(sizeof(BANG_job));
	int ref;

	if (peerArr[peer]->needsState) {
		ref = 1;

		job->data = malloc(13);
		memcpy(job->data,&ref,1);
		memcpy(job->data + 1, &fractalStateData.x, 4);
		memcpy(job->data + 5, &fractalStateData.y, 4);
		memcpy(job->data + 9, &nextJob, 4);
		job->length = 13;

	} else {
		ref = 0;

		job->data = (void *)malloc(5);
		memcpy(job->data,&ref,1);
		memcpy(job->data + 1, &nextJob, 4);
		job->length = 5;
	}

	BANG_send_job(info, job);
}

/**
 * Callback for when a peer is added.
 * \param int The id of the added peer.
 */
static void peer_added_callback(BANG_module_info* info, int peer) {

	if (peer >= peerArrSize)
		peerArr = realloc(peerArr, peerArrSize *= 2);

	peerArr[peer] = malloc(sizeof(peerContainer));
	peerArr[peer]->frame = HAS_NO_FRAME;
	peerArr[peer]->needsState = 1;

	if (jobsLeft)
		api->BANG_assert_authority_to_peer(info, api->BANG_get_my_id(info), peer);
}

/**
 * Callback for when a peer is removed.
 * \param int The id of the removed peer.
 */
static void peer_removed_callback(BANG_module_info* info, int peer) {
	//Put their job back up for someone else to do
	jobArr[peerArr[peer]->frame] = JOB_FREE;
	//Remove them from the peer array
	free(peerArr[peer]);
	peerArr[peer] = NULL;

}

/* Create a new backing pixmap of the appropriate size */
static gboolean
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  if (pixmap)
     g_object_unref (pixmap);

  pixmap = gdk_pixmap_new (widget->window,
                           widget->allocation.width,
                           widget->allocation.height,
                           -1);
  gdk_draw_rectangle (pixmap,
                      widget->style->white_gc,
                      TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gboolean
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_drawable (widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                     pixmap,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  return FALSE;
}

static int mapIterToColor(int iter, int maxIter) {
#define COL_MAX 256
	int retCol = 0;

	if (iter > (maxIter/2)) {
		retCol = 0xFF0000;
		retCol += (iter*(COL_MAX*COL_MAX)/maxIter);
		//low = red, high = white
	}
	else {
		retCol = (iter*COL_MAX/maxIter) << 20;
		//low = black, high = red
	}

#undef COL_MAX

	return retCol;
}

static void drawPixel(GtkWidget *widget, int x, int y, int color) {

	static int count = 0;
	static GdkColormap *colormap = NULL;
	if (colormap == NULL)
		colormap = gdk_colormap_get_system();
	static GdkGC *gc = NULL;
	if (gc == NULL)
		gc = gdk_gc_new(pixmap);

	GdkColor pixelColor;
	pixelColor.red = (color & 0xFF0000) >> 8;
	pixelColor.green = (color & 0xFF00);
	pixelColor.blue = (color & 0xFF) << 8;

	gdk_colormap_alloc_color(colormap, &pixelColor, FALSE, TRUE);

	gdk_gc_set_foreground(gc, &pixelColor);
	gdk_draw_point(pixmap, gc, x, y);
	gtk_widget_queue_draw_area (widget, x, y, 1, 1);

}

static gboolean button_press_event (GtkWidget *widget, GdkEventButton *event) {
	//If this is the first click, initialize everything
	if (initFlag == 1) {
		initAuthData();
		initFlag = 0;
	}

	changePosState((int)event->x, (int)event->y);
	

	//Start displaying the frames
	

}


static pixelStruct* process_frames(int numFrames, int *frame, int *numPixels) {

	pixelStruct *data = calloc((numFrames+1) * fractalStateData.winX *
				fractalStateData.winY, sizeof(pixelStruct));
	*numPixels = 0;

	double dev = 1.3;
	double midX = fractalStateData.x;
	double midY = fractalStateData.y;
	double leftBorder;
	double rightBorder;
	double bottomBorder;
	double topBorder;
	double Re_factor;
	double Im_factor;
	unsigned MaxIterations = 250;
	unsigned x;
	unsigned y;
	unsigned n;

	int i;
	for (i=0; i<numFrames; ++i) {
		dev = 1.3 / frame[i];
		MaxIterations = frame[i]*50;
		leftBorder = midX - dev;
		rightBorder = midX + dev;
		bottomBorder = midY - dev;
		topBorder = midY + dev;
		Re_factor = (rightBorder-leftBorder)/(winX-1);
		Im_factor = (topBorder-bottomBorder)/(winY-1);

		for(y=0; y<winY; ++y) {
			double c_im = topBorder - y*Im_factor;
			for(x=0; x<winX; ++x) {
				double c_re = leftBorder + x*Re_factor;
				double Z_re = c_re, Z_im = c_im;
				int isInside = 1;
				for(n=0; n<MaxIterations; ++n) {
					double Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
					if(Z_re2 + Z_im2 > 4) {
						isInside = 0;
						break;
					}
					Z_im = 2*Z_re*Z_im + c_im;
					Z_re = Z_re2 - Z_im2 + c_re;
				}
				data[*numPixels].x = x;
				data[*numPixels].y = y;
				data[*numPixels].frame = frame[i];
				data[*numPixels].color = mapIterToColor(n, MaxIterations);
				*numPixels += sizeof(pixelStruct);
			}
		}
	}

	return data;
}

void pushStateToPeers() {

	if (peerArr == NULL)
		return;

	int i;
	for (i=0; i<peerArrSize; ++i)
		peerArr[i].needsState = 1;

}

void changePosState(int x, int y) {

	fractalStateData.x = x;
	fractalStateData.y = y;

	pushStateToPeers();

}

void changeWindowState(int width, int height) {

	fractalStateData.winX = width;
	fractalStateData.winY = height;

	pushStateToPeers();

}

static void GUI_init(GtkWidget** panel, GtkWidget** panel_label) {

	*panel = gtk_vbox_new(FALSE,0);
	*panel_label = gtk_label_new("Fractal");
	window = *panel;
	label = *panel_label;
	gint width, height;
	gtk_widget_get_size_request(window, &width, &height);
	changeWindowState((int)width, (int)height);

	gtk_init (&argc, &argv);

	/* Create the drawing area */
	drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (GTK_WIDGET (drawing_area), width, height);
	gtk_box_pack_start (GTK_BOX (window), drawing_area, TRUE, TRUE, 0);

	gtk_widget_show (drawing_area);
	gtk_widget_show (window);

}

static BANG_callbacks* BANG_module_init(BANG_api *get_api) {
	api = get_api;
	BANG_callbacks *callbacks = malloc(sizeof(BANG_callbacks));

	callbacks->job_done = &job_done_callback;
	callbacks->jobs_available = &jobs_available_callback;
	callbacks->incoming_job = &incoming_job_callback;
	callbacks->outgoing_jobs = &outgoing_job_callback;
	callbacks->peer_added = &peer_added_callback;
	callbacks->peer_removed = &peer_removed_callback;

	return callbacks;
}

static void BANG_module_run(BANG_module_info *info) {
	if (window != NULL && label != NULL) {
		gtk_widget_show_all(window);
		gtk_widget_show(label);
	}

	/* Signals used to handle backing pixmap */	
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
			G_CALLBACK (expose_event), NULL);
	g_signal_connect (G_OBJECT(drawing_area),"configure_event",
			G_CALLBACK (configure_event), NULL);
	g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
			G_CALLBACK (button_press_event), NULL);
	/* Event signals */
	gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
}
