#ifndef PARCEL_RULES_H
#define PARCEL_RULES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  PARCEL_TRANSIT = 0,
  PARCEL_STORAGE = 1
} ParcelMode;

enum {
  PARCEL_EVENT_SHOCK = 1u << 0,
  PARCEL_EVENT_TILT = 1u << 1,
  PARCEL_EVENT_LIGHT_CHANGE = 1u << 2,
  PARCEL_EVENT_NEARBY_MOTION = 1u << 3
};

typedef struct {
  bool imu_valid;
  int16_t ax;
  int16_t ay;
  int16_t az;
  bool light_valid;
  uint16_t light;
  bool pir_motion;
} ParcelSample;

typedef struct {
  ParcelMode mode;
  bool light_reference_valid;
  uint16_t light_reference;
  bool shock_active;
  bool tilt_active;
  bool light_active;
  bool pir_active;
  uint8_t tilt_samples;
} ParcelRules;

void ParcelRules_Init(ParcelRules *rules, ParcelMode mode);
void ParcelRules_SetMode(ParcelRules *rules, ParcelMode mode, bool pir_motion);
uint32_t ParcelRules_Update(ParcelRules *rules, const ParcelSample *sample);

#endif
