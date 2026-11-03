#ifndef PARCEL_LIGHT_H
#define PARCEL_LIGHT_H
#include <stdbool.h>
#include <stdint.h>
/* A failed conversion must not publish a partial sample to the parcel rules. */
bool parcel_light_open(void);
bool parcel_light_read(uint16_t *raw);
#endif
