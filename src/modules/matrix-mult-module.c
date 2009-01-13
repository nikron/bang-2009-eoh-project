/**
 * \file matrix-mult-module.c
 * \author Nikhil Bysani
 * \date January 11, 2008
 *
 * \brief Multiplies two matrices together.
 */
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"../base/bang-module-api.h"
#include<gtk/gtk.h>

#define FIRST_MATRIX_FILE_TITLE "Open first matrix file"
#define SECOND_MATRIX_FILE_TITLE "Open second matrix file"

/**
 * The matrix the user wants.  This matrix might be _very_ large, so
 * this matrix is _lazy_.  So, a file descriptor along with a bitmap
 * on which rows have been read into memory, is needed to make sure
 * we know exactly what in the file we have.
 */
typedef struct {
	double **matrix;
	GIOChannel *fd;
	/**
	 * This is of size ([the side we are loading into memory] / 4) + 1
	 */
	unsigned char *bitmap;
	/**
	 * if true, rows are loaded into memory
	 * if false, columns are loaded into memory
	 */
	char bitmap_of_height;

	unsigned int width;
	unsigned int height;
} matrix;

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

static matrix *(mats[2]);

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

static void matrix_file_open(GtkFileChooserButton *button, gpointer num_matrix);

static matrix* convert_to_matrix(GIOChannel *fd);

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

/* everything in here is so gggg! 
 * TODO: Use a GError to return errors. */
static matrix* convert_to_matrix(GIOChannel *fd) {
	GError *err = NULL;
	GIOStatus status;
	matrix *mat;
	gchar *line;
	gsize length;
	gint ret_val;
	guint i;

	g_io_channel_read_line(fd,&line,&length,NULL,&err);
	if (err == NULL) {
		mat = (matrix*) g_malloc(sizeof(matrix));
		ret_val = sscanf(line,"%u,%u",&(mat->width),&(mat->height));
		g_free(line);

		if (ret_val != 2) {
			fprintf(stderr,"Here not enough scanf.\n");
			g_free(mat);
			return NULL;
		}

		/* integerity check on the file to make sure it is valid. */
		for (i = 0; i < mat->height; ++i) {
			status = g_io_channel_read_line(fd,&line,&length,NULL,&err);
			g_free(line);
			if (err != NULL || status != G_IO_STATUS_NORMAL) {
				g_free(mat);
				fprintf(stderr,"Here not enough rows.\n");
				return NULL;
			}
		}

		/* matrix seems to be okay, fill it out, and return it */

		mat->matrix = NULL;

		if (mat->width < mat->height) {
			mat->bitmap_of_height = 1;
			mat->bitmap = calloc(1,mat->height / sizeof(char) + 1);
		} else {
			mat->bitmap_of_height = 0;
			mat->bitmap = calloc(1,mat->width / sizeof(char) + 1);
		}

		mat->fd = fd;

		return mat;

	} else {
		fprintf(stderr,"Here not enough.\n");
		return NULL;
	}
}

static void matrix_file_open(GtkFileChooserButton *button, gpointer num_matrix) {
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));

	GError *error = NULL;
	GIOChannel *fd = g_io_channel_new_file(filename,"r",&error);

	if (error == NULL) {
		matrix *mat = convert_to_matrix(fd);

		if (mat) {
			gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);
			fprintf(stderr,"%d\n",*((gint*)num_matrix));
			/* long line and it's not even nessaary! */
			mats[(strcmp(FIRST_MATRIX_FILE_TITLE,gtk_file_chooser_button_get_title(button))) ? 1 : 0] = mat;
		}
	} else {
		fprintf(stderr,"%s",error->message);

	}
}

void GUI_init(GtkWidget **page, GtkWidget **page_label) {
	*page_label = gtk_label_new("Module of Matrix Multiplication");
	*page = gtk_vbox_new(FALSE,0);
	window = *page;
	label = *page_label;

	GtkWidget *matrix_one = gtk_file_chooser_button_new(FIRST_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(matrix_one),"file-set",G_CALLBACK(matrix_file_open),NULL);

	GtkWidget *matrix_two = gtk_file_chooser_button_new(SECOND_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(matrix_two),"file-set",G_CALLBACK(matrix_file_open),NULL);

	gtk_box_pack_start(GTK_BOX(window),matrix_one,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),matrix_two,TRUE,TRUE,0);

	/* The user can't interact with us until the module is running. */
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

	gtk_widget_show_all(window);
	gtk_widget_show(label);
}
