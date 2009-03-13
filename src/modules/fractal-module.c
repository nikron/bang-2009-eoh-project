/**
 * \file fractal-module.c
 * \date March 12, 2008
 *
 * \brief
 * A module to compute a series of frames of zooming in Mandelbrot fractals.
 */

//TODO: initialize totalJobs, jobsLeft

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"../base/bang-module-api.h"

char BANG_module_name[8] = "fractal";
unsigned char BANG_module_version[] = {0,0,1};
BANG_api *api;
int totalJobs;
int jobsLeft;
int totalPeers;
int peerArrSize;

typedef struct {
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
	int job;
	int x;
	int y;
	int color;
} pixelStruct;

fractalState fractalStateData;

static BANG_callbacks* BANG_module_init(BANG_api *get_api) {
	api = get_api;
	BANG_callbacks* callbacks = (BANG_callbacks*)malloc(sizeof(BANG_callbacks));
	memset(callbacks,0,sizeof(BANG_callbacks));
	return callbacks;
}

static void BANG_module_run(BANG_module_info *info) {
	api->BANG_debug_on_all_peers(info,"!\n");
}

static void GUI_init(GtkWidget** panel, GtkWidget** tabLabel) {


}

/**
 * Callback for when jobs are available for you to request
 * \param int The id of the peer with jobs available.
 */
static void jobs_available_callback(BANG_module_info* mod, int authority) {
	api->BANG_request_job(mod, authority);
}

/**
 * Callback for when a finished job is sent to you.
 * \param BANG_job* The job that has been finished.
 */
static void job_done_callback(BANG_module_info* mod, BANG_job* job) {
	buffer(job);
	//Decommission job
	jobArr[job->job_number] = -1;
	--jobsLeft;

	if (jobsLeft)
		api->BANG_assert_authority_to_peer(mod, job->authority, job->peer);

	free(mod);
}

/**
 * Callback for when a peer requests a job from you.
 * \param int The id of peer requesting a job.
 */
static void outgoing_jobs_callback(BANG_module_info* mod, int peer) {
	if (peerArr[peer] = NULL || jobsLeft == 0)
		return;

	//If next job has been finished or is being worked on
	if (jobArr[nextJob] != -1) {
		//Search for next available job
		for (i = nextJob+1; jobArr[i] != 0; ++i) {
			if (i == nextJob)
				//No more active jobs, don't do anything
				return;
			if (i+1 >= totalJobs)
				//Reset i to the beginning of the jobArr
				i = -1;
		}
		nextJob = i;
	}

	//Sign job with peer's ID
	jobArr[nextJob] = peer;
	peerArr[peer].job = nextJob;

	BANG_job *job = (BANG_job *)malloc(sizeof(BANG_job));
	if (peerArr[peer].needsState) {
		job->data = (void *)malloc(13);
		((char *)(job->data))[0] = 1;
		((int *)(job->data)+1)[0] = fractalStateData.x;
		((int *)(job->data)+1)[1] = fractalStateData.y;
		((int *)(job0>data)+1)[2] = nextJob;
		job->length = 13;
	}
	else {
		job->data = (void *)malloc(5);
		job->data[0] = 0;
		((int *)(job->data)+1)[0] = nextJob;
		job->length = 5;
	}

	BANG_send_job(mod, job);
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

static pixelStruct *processFrames(int numFrames, int *frame, int *numPixels) {

	pixelStruct *data = (pixelStruct *)calloc((numFrames+1) * winX * winY * sizeof(pixelStruct));
	*numPixels = 0;

	int i;
	for (i=0; i<numFrames; ++i) {
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
				data[*numPixels++].color = mapIterToColor(n, MaxIterations);

			}
		}
	}

	return data;
}

/**
 * Callback for when the authority sends a job to you.
 * \param BANG_job* the job that is sent to you.
 */
static void incoming_job_callback(BANG_module_info* mod, BANG_job* job) {

	//Packet: [1B state flag][4B x][4B y][4B frames]*

	int x = -1;
	int y = -1;
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
	job->data = (void *)processFrames(numFrames, frame, &numPixels);
	job->length = numPixels;
	api->BANG_finished_request(mod, job);
}

/**
 * Callback for when a peer is added.
 * \param int The id of the added peer.
 */
static void peer_added_callback(BANG_module_info* mod, int peer) {

	if (peer >= peerArrSize)
		peerArr = (peerContainer *)realloc(peerArr, peerArrSize *= 2);
	peerArr[peer] = (peerContainer *)malloc(sizeof(peerContainer));
	peerArr[peer].job = -1;
	peerArr[needsState] = 1;

	if (jobsLeft)
		api->BANG_assert_authority_to_peer(mod, api->BANG_get_my_id(mod), peer);
}

/**
 * Callback for when a peer is removed.
 * \param int The id of the removed peer.
 */
static void peer_removed_callback(BANG_module_info* mod, int peer) {
	//Put their job back up for someone else to do
	jobArr[peerArr[peer].job] = -2;
	//Remove them from the peer array
	free(peerArr[peer]);
	peerArr[peer] = NULL;

	free(mod);
}
