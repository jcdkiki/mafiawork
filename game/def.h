#ifndef RENG_DEF_H
#define RENG_DEF_H

#define RENG_MEMTRACE

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "other\glext.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define TICKS_PER_SECOND (int)25
#define SKIP_TICKS (int)(1000 / TICKS_PER_SECOND)
#define MAX_FRAMESKIP (int)5

#define UNITS_TO_METERS 0.04f
#define DELTA_TIME_IN_SECONDS (1.f / TICKS_PER_SECOND)

#ifdef _WIN32
#define KEY_SPACE VK_SPACE
#define KEY_ESCAPE VK_ESCAPE
#else
#error Unsupported OS
#endif
#endif