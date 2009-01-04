/**
 * \file bang-core.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief This implements the main public high level interface to the BANG project.  This is mainly accomplished
 * through use of BANG_init and BANG_close.
 */
#include"bang-core.h"
#include"bang-net.h"
#include"bang-com.h"
#include"bang-signals.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<getopt.h>

#define REQUIRED_ARGUEMENT 1

void BANG_init(int *argc, char **argv) {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG library starting.\n");
#endif
	///getopt!
	opterr = 0; ///We'll print our own error messages.
	optind = 1;
	char c;
	char *set_port = NULL;
	int option_index = 0;
	struct option lopts[] = {
		{"port", REQUIRED_ARGUEMENT, 0, 'p'}
	};
	while ((c = getopt_long(*argc, argv,"p:",lopts,&option_index)) != -1) {
		switch (c) {
			case 'p':
				set_port = optarg;
				break;
			default:
				///Error...
				break;
		}
	}

	BANG_sig_init();
	BANG_com_init();
	BANG_net_init(set_port,0);
}

void BANG_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG library closing.\n");
#endif
	BANG_com_close();
	BANG_net_close();
	BANG_sig_close();
}
