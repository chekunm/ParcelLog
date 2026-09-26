#include "parcel_rules.h"
#include <assert.h>
#include <stdio.h>

static ParcelSample baseline(void)
{
  ParcelSample sample = {0};
  sample.imu_valid = true;
  sample.az = 16384;
  sample.light_valid = true;
  sample.light = 1000;
  return sample;
}

int main(void)
{
  ParcelRules rules;
  ParcelSample sample = baseline();

  ParcelRules_Init(&rules, PARCEL_TRANSIT);
  assert(ParcelRules_Update(&rules, &sample) == 0);

  sample.ax = 30000;
  assert(ParcelRules_Update(&rules, &sample) == PARCEL_EVENT_SHOCK);
  assert(ParcelRules_Update(&rules, &sample) == 0);
  sample.ax = 0;
  assert(ParcelRules_Update(&rules, &sample) == 0);

  sample.ax = 12000;
  sample.az = 11000;
  assert(ParcelRules_Update(&rules, &sample) == 0);
  assert(ParcelRules_Update(&rules, &sample) == 0);
  assert(ParcelRules_Update(&rules, &sample) == PARCEL_EVENT_TILT);
  assert(ParcelRules_Update(&rules, &sample) == 0);
  sample.ax = 0;
  sample.az = 16384;
  assert(ParcelRules_Update(&rules, &sample) == 0);

  sample.light = 1700;
  assert(ParcelRules_Update(&rules, &sample) == PARCEL_EVENT_LIGHT_CHANGE);
  assert(ParcelRules_Update(&rules, &sample) == 0);
  sample.light = 1100;
  assert(ParcelRules_Update(&rules, &sample) == 0);
  sample.light = 1700;
  assert(ParcelRules_Update(&rules, &sample) == PARCEL_EVENT_LIGHT_CHANGE);

  sample.pir_motion = true;
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) == 0);
  ParcelRules_SetMode(&rules, PARCEL_STORAGE, sample.pir_motion);
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) == 0);
  sample.pir_motion = false;
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) == 0);
  sample.pir_motion = true;
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) != 0);
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) == 0);
  ParcelRules_SetMode(&rules, PARCEL_TRANSIT, sample.pir_motion);
  assert((ParcelRules_Update(&rules, &sample) & PARCEL_EVENT_NEARBY_MOTION) == 0);

  sample.imu_valid = false;
  sample.light_valid = false;
  assert(ParcelRules_Update(&rules, &sample) == 0);

  puts("parcel_rules: ok");
  return 0;
}
