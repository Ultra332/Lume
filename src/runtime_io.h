#ifndef LUME_RUNTIME_IO_H
#define LUME_RUNTIME_IO_H
#include <stdio.h>
typedef struct { FILE *input; FILE *output; } RuntimeIO;
void runtime_io_default(RuntimeIO *io);
#endif
