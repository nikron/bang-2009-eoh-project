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

static pixelStruct* process_frames(int numFrames, int *frame, int *numPixels) {
	pixelStruct *data = calloc((numFrames+1) * winX * winY, sizeof(pixelStruct));
	*numPixels = 0;

	int i;
	for (i=0; i < numFrames; ++i) {
		double dev = 1.3 / frame[i];
		double leftBorder = fractalStateData.x - dev;
		double rightBorder = fractalStateData.x + dev;
		double bottomBorder = fractalStateData.y - dev;
		double topBorder = fractalStateData.y + dev;
		double Re_factor = (rightBorder-leftBorder)/(winX-1);
		double Im_factor = (topBorder-bottomBorder)/(winY-1);
		unsigned MaxIterations = frame[i]*50 + 100;
		unsigned x;
		unsigned y;
		unsigned n;

		for(y=0; y < winY; ++y) {
			double c_im = topBorder - y*Im_factor;
			for(x=0; x < winX; ++x) {
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
				data[*numPixels++].color = mapIterToColor(n, MaxIterations);

			}
		}
	}

	return data;
}

static void GUI_init(GtkWidget** panel, GtkWidget** panel_label) {
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
}
