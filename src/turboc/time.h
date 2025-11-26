#ifndef _TIME_T
#define _TIME_T

#include "../pccore/pccore.h"

#ifdef MACOS
typedef long time_t;
#endif

time_t time(time_t *timer);

#endif