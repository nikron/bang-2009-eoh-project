/**
 * \file bang-module-api.c
 * \author Nikhil Bysani
 * \date December 24, 2009
 *
 * \brief  Fufills an api request that a module may have.
 */

#include<stdio.h>
#include"bang-module-api.h"

///TODO: Implement this.  Well, this is basically what are our app is all about.
void BANG_debug_on_all_peers(char *message) {
	fprintf(stderr,message);
}
