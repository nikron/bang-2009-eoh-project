/**
 * \file main.c
 * \author Nikhil Bysani
 * \date Decemeber 20, 2009
 *
 * \brief The main function that sets up everything and contains most of the explanatory documentation.
 */

/**
 * \mainpage BANG Engineering Open House Project
 *
 * \section intro_sec Introduction
 *
 * This project is to build an application that can distribute an arbitrary  job/task/load
 * to other computers on the network.
 *
 * The structure of the program is supposed to be divided up into layers similarly to the
 * Model View Controller (MVC) model.  However, it wont really follow MVC that much.  Most
 * things are done through the project library, known as the BANG library.
 *
 * \section lib_sec BANG Library
 *
 * The library operates in the following manner:
 *
 * An application initializes it, and then installs signal handlers.  Those handlers will be able to
 * perform the vast majority of the actions that application would want to take based on the library.
 * However, there are a few exceptions, such as loading a module which critical for this project
 * to actually do something useful (loading a module will probably generate some signals!).
 */
#include"../base/core.h"
#include"../base/bang-signals.h"
#include"../base/bang-types.h"
#include<stdio.h>
#define BDEBUG_1

void bind_suc(int signal, int sig_id, void *args) {
	fprintf(stderr,"The bind signal has been caught.\n");
	BANG_acknowledge_signal(signal,sig_id);
}

void client_con(int signal, int sig_id, void *args) {
	fprintf(stderr,"A peer has connected.\n");
	BANG_acknowledge_signal(signal,sig_id);
}
int main(int argc, char **argv) {
	BANG_init(&argc,argv);
	BANG_install_sighandler(BANG_BIND_SUC,&bind_suc);
	BANG_install_sighandler(BANG_PEER_CONNECTED,&client_con);
	while (1) {
	}
	BANG_close();
	return 0;
}
