#include "runtime_io.h"
void runtime_io_default(RuntimeIO *io) { if (io != NULL) { io->input = stdin; io->output = stdout; } }
