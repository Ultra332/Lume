#ifndef LUME_CLI_H
#define LUME_CLI_H
#include "runtime_io.h"
enum { CLI_SUCCESS = 0, CLI_LANGUAGE_ERROR = 1, CLI_USAGE_ERROR = 2 };
int cli_run(int argc, char **argv, RuntimeIO io);
#endif
