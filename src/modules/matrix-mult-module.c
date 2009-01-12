/**
 * \file matrix-mult-module.c
 * \author Nikhil Bysani
 * \date January 11, 2008
 *
 * \brief Multiplies two matrices together.
 */
#include<stdlib.h>
#include<stdio.h>
#include"../base/bang-module-api.h"
#include<gtk/gtk.h>

const char BANG_module_name[] = "matrix multiplication";
const unsigned char BANG_module_version[] = {0,0,1};
/**
 * Our id
 */
static int id;
/**
 * Our 'window' to the user.
 */
static GtkWidget *window;
/**
 * The label to to our 'window'.
 */
static GtkWidget *label;

static BANG_api api;

typedef struct {
	double *row;
	double *column;
	unsigned int length;
} matrix_multi_job;

static double multiply_row(double *column, double *row, unsigned int length);

static void job_callback(BANG_job job);

static matrix_multi_job extract_multi_job(BANG_job job);

static double multiply_row_using_job(matrix_multi_job job);

static void job_callback(BANG_job job) {
	double result = multiply_row_using_job(extract_multi_job(job));
	free(job.data);
	job.data = &result;
	/* This is pretty flimsy, but I'm pretty sure that double doesn't
	 * change size across platforms. */
	job.length = sizeof(double);
	api.BANG_finished_request(job);
}

static double multiply_row(double *row, double *column, unsigned int length) {
	unsigned int i = 0;
	double sum = 0;
	for (; i < length; ++i) {
		sum += row[i] * column[i];
	}
	return sum;
}

static double multiply_row_using_job(matrix_multi_job job) {
	return multiply_row(job.row,job.column,job.length);
}

static matrix_multi_job extract_multi_job(BANG_job job) {
	matrix_multi_job mjob;
	/* job.length should always be 2n because this module creates it */
	mjob.length = job.length / 2;
	mjob.row = (double*) job.data;
	mjob.column = ((double*) job.data) + mjob.length;

	return mjob;
}

void GUI_init(GtkWidget **page, GtkWidget **page_label) {
	*page_label = gtk_label_new("Module of Matrix Multiplication");
	*page = gtk_vbox_new(FALSE,0);

	/* The user can't interact with us until the module is running. */
	window = *page;
	label = *page_label;
}


BANG_callbacks BANG_module_init(BANG_api get_api) {
	api = get_api;

	/*TODO: Make callbacks and finish this */
	BANG_callbacks callbacks;
	callbacks.incoming_job = &job_callback;
	callbacks.outgoing_job = NULL;
	callbacks.peer_added = NULL;
	callbacks.peer_removed = NULL;

	return callbacks;
}

void BANG_module_run() {
	/* This should not change, however ids are only avialable when the program is run. */
	id = api.BANG_get_my_id();

	gtk_widget_show(window);
	gtk_widget_show(label);
}
