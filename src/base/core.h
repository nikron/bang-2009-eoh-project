#ifndef __CORE_H
#define __CORE_H
typedef void BANGSignalHandler(int,void **args);

void BANG_init(int *argc, char **argv);
int BANG_load_module(char *path);
int BANG_install_signal(int signal, BANGSignalHandler);
void* BANG_find_peers();
#endif
