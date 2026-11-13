#include "parcel_print.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static char wire[1024];
static unsigned used;
int board_put_char(int ch) { assert(used<sizeof wire-1);wire[used++]=(char)ch;wire[used]=0;return ch; }
int main(void) {
    int n=parcel_printf("%s %u %d %lu %ld %02x %%\r\n","storage",65535u,-32768,4294967295ul,-2147483647l-1,0x8u);
    assert(!strcmp(wire,"storage 65535 -32768 4294967295 -2147483648 08 %\r\n"));
    assert(n==(int)strlen(wire));
    used=0;parcel_printf("LOG slot=%u seq=%lu boot=%lu ms=%lu kind=%s mode=%u valid=%u adc=%u xyz=%d,%d,%d crc=ok\r\n",127u,128ul,1ul,3600000ul,"inspection",1u,3u,4095u,-32768,0,32767);
    assert(!strcmp(wire,"LOG slot=127 seq=128 boot=1 ms=3600000 kind=inspection mode=1 valid=3 adc=4095 xyz=-32768,0,32767 crc=ok\r\n"));
    puts("Integer wire formatter: signed limits, widths, UART LOG fields and counts passed");
}
