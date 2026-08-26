#ifndef LUME_LEARN_H
#define LUME_LEARN_H

#include "runtime_io.h"

int learn_cli(int argc, char **argv, RuntimeIO io, const char *executable_path);
int learn_cli_from_directory(int argc, char **argv, RuntimeIO io,
                             const char *content_directory);

#endif
