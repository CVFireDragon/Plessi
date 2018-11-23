#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <switch.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>

#define BufSize 0x500000
#define MIN(x, y) (((x) < (y)) ? (x) : (y))