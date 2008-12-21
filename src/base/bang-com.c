#include"bang-com.h"
#include"bang-signals.h"
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>

void BANG_connection_signal_handler(int signal,int sig_id,void* socket) {
	///TODO: Create a slave thread
	shutdown(*((int*)socket),SHUT_RDWR);
	BANG_acknowledge_signal(signal,sig_id);
}
