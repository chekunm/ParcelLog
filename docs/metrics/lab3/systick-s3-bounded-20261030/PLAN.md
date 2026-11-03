# ParcelLog: готовий обмежений S3 та IMU fault

Конфігурація одного запуску Wokwi до 175 секунд wall. Незмінні ELF SHA256 2b22b7cc8a8a7a47687667399a58bd45a28b6be3d93b43c37b479bc78aabe6b2, HEX 3f46d586a9dd0b08d30b4eb74cc10fc065df1ba17e330e30bc6eefd53b230858. RFC2217 4505, GDB вимкнений, порт 33345 не використовується.

Fixture SHA256 c5e5d1ad020d5c5d7558d0617bc54b31183269ed41b344df0a533c9412a0a348: генератор 0.937 Гц на PB1, PB0=storage, D0–D7 — SAMPLE, FLASH, TX, PIR, ADC, RESPONSE, RX, MODE. Це тестові фронти генератора. Root diagram оригінал SHA256 7954276423c35a9e0ef4382b057e9a4b38ee329e943135fa0d6d25d27b521cb5. wokwi.toml SHA256 c7a07d41befffb00b95df04bbfcab28ca602c621d7086715314b38cd9c6b9d92, VCD destination docs/captures/lab3/systick-gate-20261030.vcd. Capture експортується після Stop; його конфігурація визначається зазначеними SHA256.

Schedule SHA256 a589eac538197927e402504a57d4032aa995d95efa04610224e1761268132832: 524 erase CONFIRM з 5 до 145 с wall, змінні інтервали 183/227/269/311/347 мс для розсіювання фази, без шквалу status; на 150 с imuaddr 0x69, 155 status, 158 imuaddr 0x68, 165 status, 168 stats. Collector --seconds 175, preconnect до start, записати командні host times і UART session JSON. Це задум, не гарантоване фазове покриття, кількість фактичних erase і збігів чи успіх операції. Контроль bufferSize та фактичної тривалості/повноти VCD; при обрізанні чесно вказати. Номінальна тривалість collection — 175 с.

Валідний збіг: фронт PB1 всередині завершеного FLASH-маркера, storage на фронті, перевірені часово узгоджені nearby_motion та RESPONSE до наступного фронту, без змішування з іншою подією; неоднозначні окремо. Невдала HAL erase та scan — фактична операція, не підтверджене успішне стирання чи фізичний stall. До 30 не підганяти. IMU fault вважається перевіреним тільки при фактичних UART результатах NACK/втрати й наступного відновлення; якщо цього немає — невідомо. Адресний NACK не замінює окремої ін'єкції SDA low або перерваного запису.


