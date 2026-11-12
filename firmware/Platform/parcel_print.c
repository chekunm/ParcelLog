#include "parcel_print.h"
#include "board.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

/* Integer-only wire formatter. All callers hold the application mutex, except
 * initialization before scheduler start. No libc FILE/heap or shared va state. */
int parcel_printf(const char *format, ...) {
    va_list args; va_start(args, format); int count = 0;
    while (*format) {
        if (*format != '%') { board_put_char(*format++); ++count; continue; }
        ++format; bool zero = *format == '0'; if (zero) ++format;
        unsigned width=0; while (*format>='0' && *format<='9') width=width*10u+(unsigned)(*format++-'0');
        bool wide=*format=='l'; if (wide) ++format;
        char type=*format++;
        if (type=='s') {
            const char *s=va_arg(args,const char *); while (*s) {board_put_char(*s++);++count;}
        } else if (type=='%') {board_put_char('%');++count;}
        else {
            uint32_t value;
            if (type=='d') {
                int32_t signed_value=wide ? va_arg(args,long) : va_arg(args,int);
                if (signed_value<0) {board_put_char('-');++count;value=0u-(uint32_t)signed_value;}
                else value=(uint32_t)signed_value;
            } else value=wide ? va_arg(args,unsigned long) : va_arg(args,unsigned);
            unsigned base=type=='x' ? 16u : 10u; char digits[10];unsigned n=0;
            do {digits[n++]="0123456789abcdef"[value%base];value/=base;} while (value);
            while (width>n) {board_put_char(zero?'0':' ');++count;--width;}
            while (n) {board_put_char(digits[--n]);++count;}
        }
    }
    va_end(args);return count;
}
