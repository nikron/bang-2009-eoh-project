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
#define RESULTANT_MATRIX_FILE_TITLE "Save resultant matrix"

#define FIRST_MATRIX 0
#define SECOND_MATRIX 1
#define PRODUCT_MATRIX 2

#define NUM_MATRICES

enum orientation {
	LOAD_ROWS = 0,
	LOAD_COLUMNS = 1
};

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
	 * This is of size ([height] / 4) + 1
	 */
	unsigned char *bitmap;

	unsigned int width;
	unsigned int height;
} matrix;

typedef struct {
	double *row;
	double *column;
	unsigned int length;
} matrix_multi_job;

typedef struct {
	GtkButton *multiply_button;
	matrix *(matrices[2]);
} file_chooser_data;


const char BANG_module_name[] = "matrix multiplication";
const unsigned char BANG_module_version[] = {0,0,1};
/**
 * Our id
 */
static int id = -1;
/**
 * Our 'window' to the user.
 */
static GtkWidget *window = NULL;
/**
 * The label to to our 'window'.
 */
static GtkWidget *label = NULL;

static matrix *(matrices[NUM_MATRICES]) = { NULL, NULL, NULL };

static BANG_api api;

static double multiply_row(double *column, double *row, unsigned int length);

static void job_callback(BANG_job job);

static void jobs_available(int id);

static matrix_multi_job extract_multi_job(BANG_job job);

static double multiply_row_using_job(matrix_multi_job job);

static void matrix_file_callback(GtkFileChooserButton *button, gpointer d);

static void matrices_do_multiply(GtkButton *button, gpointer m);

static matrix* convert_to_matrix(GIOChannel *fd);

static matrix* new_matrix(GIOChannel *fd);

static matrix* new_matrix_with_dimensions(int width, int height, GIOChannel *fd);

static void free_matrix(matrix *mat);

static matrix* new_matrix(GIOChannel *fd) {
	matrix *mat = g_malloc0(sizeof(matrix));
	mat->fd = fd;

	return mat;
}

static void matrix_set_dimensions(matrix *mat, int width, int height) {
	mat->width = width;
	mat->height = height;

	if (mat->bitmap != NULL)
		g_free(mat->bitmap);

	mat->bitmap = g_malloc0((height / 4) + 1);
}

static matrix* new_matrix_with_dimensions(int width, int height, GIOChannel *fd) {
	matrix *mat = new_matrix(fd);

	matrix_set_dimensions(mat,width,height);

	return mat;
}

static void free_matrix(matrix *mat) {
	g_free(mat->matrix);
	mat->matrix = NULL;

	g_free(mat->bitmap);
	mat->bitmap = NULL;
	mat->width = 0;
	mat->height = 0;

	g_free(mat);
}

static void job_callback(BANG_job job) {
	double result = multiply_row_using_job(extract_multi_job(job));
	free(job.data);
	job.data = &result;
	/* This is pretty flimsy, but I'm pretty sure that double doesn't
	 * change size across platforms. */
	job.length = sizeof(double);
	api.BANG_finished_request(job);
}

static void jobs_available(int id) {
	/* We'll accept all incoming jobs, and tell them to use the callback. */
	api.BANG_request_job(id,0);
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
	guint i, width, height;

	g_io_channel_read_line(fd,&line,&length,NULL,&err);
	if (err == NULL && length > 0) {
		ret_val = sscanf(line,"%u,%u",&width,&height);
		g_free(line);

		if (ret_val != 2) {
			return NULL;
		}

		/* file check to make sure it is valid. */
		for (i = 0; i < height; ++i) {
			status = g_io_channel_read_line(fd,&line,&length,NULL,&err);
			g_free(line);
			if (err != NULL || status != G_IO_STATUS_NORMAL) {
				return NULL;
			}
		}

		/* matrix seems to be okay, fill it out, and return it */
		mat = new_matrix_with_dimensions(width,height,fd);

		return mat;

	} else {
		return NULL;
	}
}

static void matrices_do_multiply(GtkButton *button, gpointer m) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new (RESULTANT_MATRIX_FILE_TITLE,
			NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL);

	GIOChannel *fd = NULL;
	GError *err = NULL;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		fd = g_io_channel_new_file(filename,"w",&err);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	
	if (fd && err == NULL) {
		gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);
		matrices[FIRST_MATRIX] = ((matrix**)m)[0];
		matrices[SECOND_MATRIX] = ((matrix**)m)[1];
		matrices[PRODUCT_MATRIX] = new_matrix(fd);
	}
}

static void matrix_file_callback(GtkFileChooserButton *button, gpointer d) {
	file_chooser_data *data = d;
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
	if (filename == NULL) return;

	int  matrix_number;
	char *file_permissions = "r";

	GError *error = NULL;

	if (!strcmp(FIRST_MATRIX_FILE_TITLE,gtk_file_chooser_button_get_title(button))) {
		matrix_number = 0;

	} else {
		matrix_number = 1;

	} 

	GIOChannel *fd = g_io_channel_new_file(filename,file_permissions,&error);

	if (error == NULL && fd) {
		if (data->matrices[matrix_number]) {
			free_matrix(data->matrices[matrix_number]);
		}

		data->matrices[matrix_number] = convert_to_matrix(fd);
	}

	/* Make sure the matrices are multiplicable. */
	if (data->matrices[0] && 
			data->matrices[1] && 
			data->matrices[0]->height == data->matrices[1]->width) {

		gtk_widget_set_sensitive(GTK_WIDGET(data->multiply_button),TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(data->multiply_button),FALSE);
	}

	g_free(filename);
}

void GUI_init(GtkWidget **page, GtkWidget **page_label) {
	*page_label = gtk_label_new("Module of Matrix Multiplication");
	*page = gtk_vbox_new(FALSE,0);
	window = *page;
	label = *page_label;
	file_chooser_data *data = g_malloc0(sizeof(file_chooser_data));

	data->multiply_button = GTK_BUTTON(gtk_button_new_with_label("Multiply Matrices"));
	gtk_widget_set_sensitive(GTK_WIDGET(data->multiply_button),FALSE);
	g_signal_connect(G_OBJECT(data->multiply_button),"clicked",G_CALLBACK(matrices_do_multiply),data->matrices);

	GtkWidget *mone_label = gtk_label_new("First Matrix File:");
	GtkWidget *matrix_one = gtk_file_chooser_button_new(FIRST_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(matrix_one),"file-set",G_CALLBACK(matrix_file_callback),data);

	GtkWidget *mtwo_label = gtk_label_new("Second Matrix File:");
	GtkWidget *matrix_two = gtk_file_chooser_button_new(SECOND_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(matrix_two),"file-set",G_CALLBACK(matrix_file_callback),data);

	gtk_box_pack_start(GTK_BOX(window),mone_label,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),matrix_one,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),mtwo_label,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),matrix_two,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),GTK_WIDGET(data->multiply_button),FALSE,FALSE,0);

	/* The user can't interact with us until the module is running. */
}


BANG_callbacks BANG_module_init(BANG_api get_api) {
	api = get_api;

	/*TODO: Make callbacks and finish this */
	BANG_callbacks callbacks;
	callbacks.jobs_available = &jobs_available;
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
