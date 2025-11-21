#include "time.h"

time_t time(time_t *timer){
    time_t seconds = (time_t)(pccore.time / 1000);

    if (timer != NULL) {
        *timer = seconds;
    }
    return seconds;
}