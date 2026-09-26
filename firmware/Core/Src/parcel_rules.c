#include "parcel_rules.h"

/* Raw MPU6050 acceleration at +/-2 g: 16384 counts per g. */
#define SHOCK_ON 27853u
#define SHOCK_OFF 22937u
#define TILT_ON 10649u
#define TILT_OFF 8192u
#define LIGHT_ON 500u
#define LIGHT_OFF 300u
#define TILT_CONFIRM_SAMPLES 3u

static uint32_t abs_axis(int16_t value)
{
  return value < 0 ? (uint32_t)(-(int32_t)value) : (uint32_t)value;
}

static uint32_t abs_diff(uint16_t a, uint16_t b)
{
  return a > b ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

void ParcelRules_Init(ParcelRules *rules, ParcelMode mode)
{
  *rules = (ParcelRules){0};
  rules->mode = mode;
}

void ParcelRules_SetMode(ParcelRules *rules, ParcelMode mode, bool pir_motion)
{
  rules->mode = mode;
  /* A pulse already high when storage begins is not a new storage event. */
  rules->pir_active = pir_motion;
}

uint32_t ParcelRules_Update(ParcelRules *rules, const ParcelSample *sample)
{
  uint32_t events = 0;

  if (sample->imu_valid) {
    uint32_t x = abs_axis(sample->ax);
    uint32_t y = abs_axis(sample->ay);
    uint32_t z = abs_axis(sample->az);
    uint32_t peak = x > y ? x : y;
    peak = peak > z ? peak : z;

    if (peak >= SHOCK_ON && !rules->shock_active) {
      rules->shock_active = true;
      events |= PARCEL_EVENT_SHOCK;
    } else if (peak < SHOCK_OFF) {
      rules->shock_active = false;
    }

    if (x >= TILT_ON || y >= TILT_ON) {
      if (rules->tilt_samples < TILT_CONFIRM_SAMPLES) {
        rules->tilt_samples++;
      }
      if (rules->tilt_samples == TILT_CONFIRM_SAMPLES && !rules->tilt_active) {
        rules->tilt_active = true;
        events |= PARCEL_EVENT_TILT;
      }
    } else if (x < TILT_OFF && y < TILT_OFF) {
      rules->tilt_samples = 0;
      rules->tilt_active = false;
    }
  }

  if (sample->light_valid) {
    if (!rules->light_reference_valid) {
      rules->light_reference = sample->light;
      rules->light_reference_valid = true;
    } else {
      uint32_t change = abs_diff(sample->light, rules->light_reference);
      if (change >= LIGHT_ON && !rules->light_active) {
        rules->light_active = true;
        events |= PARCEL_EVENT_LIGHT_CHANGE;
      } else if (change < LIGHT_OFF) {
        rules->light_active = false;
      }
    }
  }

  if (rules->mode == PARCEL_STORAGE && sample->pir_motion && !rules->pir_active) {
    events |= PARCEL_EVENT_NEARBY_MOTION;
  }
  rules->pir_active = sample->pir_motion;

  return events;
}
