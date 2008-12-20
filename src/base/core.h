#ifndef __CORE_H
#define __CORE_H

#define DEFAULT_PORT "7878"

typedef void BANGSignalHandler(int,void **args);

void BANG_init(int *argc, char **argv);
void BANG_close();
int BANG_load_module(char *path);
int BANG_install_signal(int signal, BANGSignalHandler);
void* BANG_find_peers();
#endif
