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
 * The matrix the user wants.   Later, we should implement this
 * so that the matrix can be lazy.
 */
typedef struct {
	double **matrix;
	GIOChannel *fd;
	/**
	 * This is of size ([height] / 4) + 1
	 */
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
	GtkWidget *mone;
	GtkWidget *mtwo;
	matrix *(matrices[2]);
} file_chooser_data;


const char BANG_module_name[] = "matrix multiplication";
const unsigned char BANG_module_version[] = {0,0,1};
/**
 * Our id
 */
static int my_id = -1;
/**
 * Our 'window' to the user.
 */
static GtkWidget *window = NULL;
/**
 * The label to to our 'window'.
 */
static GtkWidget *label = NULL;

static matrix *(matrices[NUM_MATRICES]) = { NULL, NULL, NULL };

enum job_status {
	JOB_ABANDONED = -1,
	JOB_BEING_WORKED_ON  = -2,
	JOB_DONE = -3
};

static int *jobs = NULL;
static int current = -1;
static int jobs_being_worked_on = -1;

static BANG_api *api;

static double multiply_row(double *column, double *row, unsigned int length);

static void jobs_done_callback(BANG_module_info *info, BANG_job *job);

static void job_incoming_callback(BANG_module_info *info, BANG_job *job);

static void job_outgoing_callback(BANG_module_info *info, int id);

static void jobs_available_callback(BANG_module_info *info, int id);

static void peer_added_callback(BANG_module_info *info, int id);

static void peer_removed_callback(BANG_module_info *info, int id);

static BANG_job* new_BANG_job();

static BANG_job* new_BANG_job_with_num(int peer, int job_num);

static void free_jobs_held_by(int id);

static matrix_multi_job extract_multi_job(BANG_job *job);

static double multiply_row_using_job(matrix_multi_job job);

static void matrix_file_callback(GtkFileChooserButton *button, gpointer d);

static void matrices_do_multiply(GtkButton *button, gpointer m);

static matrix* convert_to_matrix(GIOChannel *fd);

static matrix* new_matrix(GIOChannel *fd);

static matrix* new_matrix_with_dimensions(int width, int height, GIOChannel *fd);

static void free_matrix(matrix *mat);

static matrix* read_in_row(matrix *mat, unsigned int row);

static double* get_row_matrix(matrix *mat, unsigned int row);

static double* get_col_matrix(matrix *mat, unsigned int col);

static int get_row_num_with_job_number(matrix *mat,int job_num);

static int get_col_num_with_job_number(matrix *mat,int job_num);

static void set_cell_matrix(matrix *mat, double value, unsigned int width, unsigned int height);

static void set_cell_matrix_with_job(BANG_job *job);

static matrix* new_matrix(GIOChannel *fd) {
	matrix *mat = g_malloc0(sizeof(matrix));
	mat->fd = fd;

	return mat;
}

static void matrix_set_dimensions(matrix *mat, int width, int height) {
	int i;
	/* gchar *buf; */
	GError *err;

	mat->width = width;
	mat->height = height;
	/* char reading; */

	if (mat->matrix != NULL)
		g_free(mat->matrix);

	mat->matrix = g_malloc0(height * sizeof(double*));

	g_io_channel_seek_position(mat->fd,0,G_SEEK_SET,&err);

	for (i = 0; i < height; ++i) {
		read_in_row(mat,i);
	}
}

static matrix* read_in_row(matrix *mat, unsigned int row) {
	mat->matrix[row] = g_malloc(mat->width * sizeof(double));
	/* TODO: Actually read in!*/
	return mat;
}

static matrix* new_matrix_with_dimensions(int width, int height, GIOChannel *fd) {
	matrix *mat = new_matrix(fd);

	matrix_set_dimensions(mat,width,height);

	return mat;
}

static BANG_job* new_BANG_job_with_num(int peer, int job_num) {
	BANG_job *job = g_malloc0(sizeof(BANG_job));
	job->job_number = job_num;
	job->peer = peer;
	job->authority = my_id;

	unsigned int row_num, col_num;
	double *row, *col;

	row_num = get_row_num_with_job_number(matrices[PRODUCT_MATRIX],job_num);
	col_num = get_col_num_with_job_number(matrices[PRODUCT_MATRIX],job_num);

	row = get_row_matrix(matrices[FIRST_MATRIX],row_num);
	col = get_col_matrix(matrices[SECOND_MATRIX],col_num);

	/* Size of the information we are gonna send. */
	job->length = matrices[FIRST_MATRIX]->height * 2 * sizeof(double);

	job->data = g_malloc(job->length);
	memcpy(job->data,row,job->length / 2);
	memcpy(job->data + job->length / 2,col,job->length / 2);

	g_free(row);
	g_free(col);

	return job;
}

static BANG_job* new_BANG_job(int peer) {
	/* TODO: use locks! */
	int i;
	for (i = current; i < current + jobs_being_worked_on; ++i) {
		if (jobs[i] == JOB_ABANDONED) {
			jobs[i] = JOB_BEING_WORKED_ON;
			return new_BANG_job_with_num(peer,i);
		}
	}
	++jobs_being_worked_on;
	jobs[jobs_being_worked_on] = JOB_BEING_WORKED_ON;
	return new_BANG_job_with_num(peer,jobs_being_worked_on);
}

static void free_jobs_held_by(int peer) {
	/* TODO: uses locks! */
	int i;
	for (i = current; i < current + jobs_being_worked_on; ++i) {
		if (peer == jobs[i]) {
			jobs[i] = JOB_ABANDONED;
		}
	}
}

static void free_matrix(matrix *mat) {
	unsigned int i = 0;
	for (i = 0; i < mat->height; ++i) {
		free(mat->matrix[i]);
	}
	g_free(mat->matrix);
	mat->matrix = NULL;

	mat->width = 0;
	mat->height = 0;

	g_free(mat);
}
static int get_row_num_with_job_number(matrix *mat, int job_num) {
	return mat->height % job_num;
}

static int get_col_num_with_job_number(matrix *mat,int job_num) {
	return mat->height / job_num;
}

static void set_cell_matrix(matrix *mat, double value, unsigned int col, unsigned int row) {
	/* TODO: locks! */
	if (mat->matrix[row] == NULL) {
		mat->matrix[row]= g_malloc(mat->height * sizeof(double));
	}

	mat->matrix[row][col] = value;
}

static double* get_row_matrix(matrix *mat, unsigned int row_num) {
	double* row = g_malloc(mat->width * sizeof(double));
	memcpy(row,mat->matrix[row_num],mat->width * sizeof(double));
	return row;
}

static double* get_col_matrix(matrix *mat, unsigned int col_num) {
	unsigned int i = 0;
	double *col = g_malloc(mat->height * sizeof(double));

	for (i = 0; i < mat->height; ++i) {
		col[i] = mat->matrix[i][col_num];
	}

	return col;
}

static void set_cell_matrix_with_job(BANG_job *job) {
	int width, height;
	width = get_row_num_with_job_number(matrices[PRODUCT_MATRIX],job->job_number);
	height = get_col_num_with_job_number(matrices[PRODUCT_MATRIX],job->job_number);
	double cell = *((double*)job->data);

	if (width != -1 && height != -1) {
		set_cell_matrix(matrices[PRODUCT_MATRIX],cell,width,height);
	}
}

static void jobs_done_callback(BANG_module_info *info, BANG_job *job) {
	if (jobs[job->job_number] != JOB_DONE) {
		jobs[job->job_number] = JOB_DONE;
		set_cell_matrix_with_job(job);
	}
	/* suppress compiler warning
	 * TODO: find a better way */
	info = NULL;
}

static void job_incoming_callback(BANG_module_info *info, BANG_job *job) {
	double *result = g_malloc(sizeof(double));
	*result = multiply_row_using_job(extract_multi_job(job));
	free(job->data);
	job->data = result;
	/* This is pretty flimsy, but I'm pretty sure that double doesn't
	 * change size across platforms. */
	job->length = sizeof(double);
	api->BANG_finished_request(info,job);
}

static void job_outgoing_callback(BANG_module_info *info, int peer) {
	BANG_job *job = new_BANG_job(peer);
	api->BANG_send_job(info,job);
}

static void jobs_available_callback(BANG_module_info *info, int id) {
	/* We'll accept all incoming jobs, and tell them to use the callback. */
	api->BANG_request_job(info,id);
}

static void peer_added_callback(BANG_module_info *info, int id) {
	if (jobs) {
		api->BANG_assert_authority_to_peer(info,my_id,id);
	}
}

static void peer_removed_callback(BANG_module_info *info, int id) {
	free_jobs_held_by(id);
	/* SUPPRESS WARNINGS OH GOD I FAIL. */
	info = NULL;
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

static matrix_multi_job extract_multi_job(BANG_job *job) {
	matrix_multi_job mjob;
	/* job.length should always be 2n because this module creates it */
	mjob.length = job->length / 2;
	mjob.row = (double*) job->data;
	mjob.column = ((double*) job->data) + mjob.length;

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

static void matrices_do_multiply(GtkButton *button, gpointer d) {
	file_chooser_data *data = d;
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
		gtk_widget_set_sensitive(data->mone,FALSE);
		gtk_widget_set_sensitive(data->mtwo,FALSE);

		matrices[FIRST_MATRIX] = data->matrices[0];
		matrices[SECOND_MATRIX] = data->matrices[1];
		matrices[PRODUCT_MATRIX] = new_matrix_with_dimensions(data->matrices[0]->height,
				data->matrices[1]->width,
				fd);

		/* Keep track of the jobs sent out by an array of ids for each job. */
		jobs = g_malloc(matrices[PRODUCT_MATRIX]->height * matrices[PRODUCT_MATRIX]->width * sizeof(int));
		current = 0;

		/* api->BANG_assert_authority(my_id);
		 * TODO: Need to this on how this will work. */
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
			data->matrices[0]->width == data->matrices[1]->height) {

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
	g_signal_connect(G_OBJECT(data->multiply_button),"clicked",G_CALLBACK(matrices_do_multiply),data);

	GtkWidget *mone_label = gtk_label_new("First Matrix File:");
	data->mone = gtk_file_chooser_button_new(FIRST_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(data->mone),"file-set",G_CALLBACK(matrix_file_callback),data);

	GtkWidget *mtwo_label = gtk_label_new("Second Matrix File:");
	data->mtwo = gtk_file_chooser_button_new(SECOND_MATRIX_FILE_TITLE,GTK_FILE_CHOOSER_ACTION_OPEN);
	g_signal_connect(G_OBJECT(data->mtwo),"file-set",G_CALLBACK(matrix_file_callback),data);

	gtk_box_pack_start(GTK_BOX(window),mone_label,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),data->mone,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),mtwo_label,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),data->mtwo,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(window),GTK_WIDGET(data->multiply_button),FALSE,FALSE,0);

	/* The user can't interact with us until the module is running. */
}


BANG_callbacks BANG_module_init(BANG_api *get_api) {
	api = get_api;

	/*TODO: Make callbacks and finish this */
	BANG_callbacks callbacks;
	callbacks.job_done = &jobs_done_callback;
	callbacks.jobs_available = &jobs_available_callback;
	callbacks.incoming_job = &job_incoming_callback;
	callbacks.outgoing_jobs = &job_outgoing_callback;
	callbacks.peer_added = &peer_added_callback;
	callbacks.peer_removed = &peer_removed_callback;

	return callbacks;
}

void BANG_module_run(BANG_module_info *info) {
	/* This should not change, however ids are only avialable when the program is run. */
	my_id = api->BANG_get_my_id(info);

	gtk_widget_show_all(window);
	gtk_widget_show(label);
}
