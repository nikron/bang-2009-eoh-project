#ifndef __BANG_TYPES_H
#define __BANG_TYPES_H

/// A signal handler function, it receives the BANGSIGNUM, and arguments depending on the signal.
typedef void BANGSignalHandler(int,void **args);

#endif
