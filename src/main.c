#include "cli.h"
int main(int argc, char **argv) { RuntimeIO io; runtime_io_default(&io); return cli_run(argc, argv, io); }
