# ParcelLog, ЛР2: підключення

| MCU | Блок | Роз'єм | Сигнал |
|---|---|---|---|
| PB8 | AF6 I2C1 SCL | CN5.10 D15, CN10.3 | MPU SCL, LA D0 |
| PB9 | AF6 I2C1 SDA | CN5.9 D14, CN10.5 | MPU SDA, LA D1 |
| PA0 | ADC1 IN0 | CN8.1 A0 | LDR AO |
| PB0 | GPIO input | CN5.3 D10 | фізичний перемикач transit/storage, LA D7 |
| PB1 | GPIO input | CN8.4 A3 | PIR OUT, LA D3 |
| PA2 | AF1 USART2 TX | CN7.37 VCP | monitor, LA D2 |
| PA3 | AF1 USART2 RX | CN10.10 VCP | commands, LA D6 |
| PA4 | GPIO output | CN8.3 A2 | ADC marker, LA D4 |
| PA5 | GPIO output | CN5.6 D13 | LED, LA D5 |

MPU AD0=GND, адреса 0x68. Обидві I2C лінії мають 4,7 кОм до 3V3. Всі GND спільні. В моделі сенсори живляться від 3V3; це не доводить, що довільний фізичний PIR module із регулятором працює від 3,3 V. Для фізичного варіанта потрібні документований модуль, перевірка VCC/OUT і вимірювання струму. PA13/PA14 не зайняті. PA5 зайнятий LED; SPI/PWM/DMA transfers відсутні.

Джерела: DS13867 Rev4 §4, AF port A/B; UM2953 Rev1 fig.7 p.19, table11 p.20, VCP §6.9 p.16. UM2953 table12 p.23 має переставлені SDA/SCL біля PB8/PB9 відносно table11. Використано PB8=SCL та PB9=SDA за datasheet і table11. Arduino D0/D1 є USART1 PB7/PB6, не USART2. Підключення визначене MCU pin в diagram.json.
