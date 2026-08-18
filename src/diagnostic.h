#ifndef LUME_DIAGNOSTIC_H
#define LUME_DIAGNOSTIC_H
#include <stdio.h>
#include "error.h"
#include "source.h"
void diagnostic_render(FILE *stream, const Source *source, const LumeError *error);
#endif
