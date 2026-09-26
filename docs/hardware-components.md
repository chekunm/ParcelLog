# Компоненти та підключення

Схема використовує віртуальну плату STM32C031C6 і компоненти Wokwi. Реальних модулів немає, тому наведені аналоги не є виміряними компонентами цього прототипу. Точні з'єднання задані у [`diagram.json`](../diagram.json).

| Компонент | Інтерфейс і контакти STM32 | Що використовує прошивка | Джерело |
| --- | --- | --- | --- |
| MPU6050 | I²C через GPIO HAL: PB8/SCL, PB9/SDA; підтяжки 4,7 кОм до 3V3, GND, AD0→GND | Адреса 0x68, `WHO_AM_I` 0x75, пробудження через `PWR_MGMT_1` 0x6B, діапазон ±2 g через `ACCEL_CONFIG` 0x1C, читання прискорення з 0x3B | [Wokwi MPU6050, Pin names та Attributes](https://docs.wokwi.com/parts/wokwi-mpu6050); [TDK MPU-6050, Electrical Characteristics](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) |
| Фоторезистор Wokwi | ADC1_IN0: PA0; 3V3, GND | Аналоговий вихід AO; порівняння 12-бітного показу з початковим | [Wokwi photoresistor, Pin names та Operation](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor) |
| PIR Wokwi | GPIO: PB1; 3V3, GND | Цифровий вихід OUT, фронт активного сигналу лише у `storage` | [Wokwi PIR, Pin names та Using the sensor](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor) |
| Перемикач режиму | GPIO: PB0, контакти GND/PB0/3V3 | 0 — `transit`, 1 — `storage`; внутрішня підтяжка до GND | [Wokwi slide switch, Pin names](https://docs.wokwi.com/parts/wokwi-slide-switch) |
| UART і світлодіод плати | USART2: PA2/TX, PA3/RX; LED: PA5 | Журнал 115200 бод і індикатор роботи | [Wokwi Nucleo-C031C6, Pinout](https://docs.wokwi.com/parts/board-st-nucleo-c031c6) |

## Межі аналогії з фізичними модулями

- [TDK MPU-6050](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) має живлення VDD 2,375–3,46 В і I²C; це обґрунтовує вибір 3,3 В у симуляції. Параметри конкретної плати-модуля залежать від її схеми.
- Для фоторезистора можливий реальний елемент [Senba GL5528, таблиця φ5](https://ru.senbasensor.com/visible-light-sensor/56.html), але Wokwi моделює власний аналоговий модуль з LDR та резистором 10 кОм. Параметри GL5528 не слід видавати за точну характеристику симулятора.
- Реальний [Joy-IT SEN-HC-SR501, datasheet, стор. 1](https://joy-it.net/files/files/Produkte/SEN-HC-SR501/SEN-HC-SR501-Datasheet-23.09.2020.pdf) потребує **5 В** живлення та має логічний рівень 3 В. Віртуальний PIR у `diagram.json` під'єднаний до 3V3; таке підключення не можна переносити на фізичний SEN-HC-SR501 без зміни схеми живлення.

У Wokwi перевірено запуск з `mpu6050_ready` та `imu=ok`. Споживання струму всієї схеми та форма сигналів I²C не виміряні; фізичної плати немає. Для звіту слід зробити власні кадри Wokwi.
