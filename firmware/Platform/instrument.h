#ifndef PARCEL_INSTRUMENT_H
#define PARCEL_INSTRUMENT_H
#include <stdbool.h>
#include <stdint.h>
void instrument_early(void);
void instrument_init(void);
uint32_t instrument_now(void);
uint32_t instrument_stack(void);
uint32_t instrument_heap(void);
void instrument_heap_usage(uint32_t *used, uint32_t *free_bytes);
void instrument_probe(unsigned pin, bool high);
void instrument_idle(void);
void instrument_irq_enter(void);
void instrument_irq_leave(void);
uint64_t instrument_sleep_us(void);
#endif
