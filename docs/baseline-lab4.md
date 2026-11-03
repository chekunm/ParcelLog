# Базова лінія ParcelLog для ЛР4

Це базова лінія super loop для повторюваного порівняння з FreeRTOS. Базову лінію виміряно у Wokwi; наведені нижче обмеження залишаються відкритими.

## Незмінні умови

STM32C031C6, HSI 48 МГц, UART 115200 8N1, I2C 100 кГц, той самий ELF/source manifest для S1–S3, -Os, SysTick 1 мс, sample 100 мс. Форма B: Wokwi. PA6/PA7/PA8 та LA канали див. captures/lab3/README.md. Засіб вимірювання не замінювати на host arrival timestamps. Build manifest, банер, UART і VCD визначають походження вимірювань.

## Відтворення

1. `make build`, `make test-linux`; покриття за методикою `tools/HEADLESS-TOOLS.md`; перевірити source-manifest і банер.
2. S1: стандартний diagram, transit, PIR LOW, IMU 0/0/1g, LDR 500 lux; 310 с, stats на 305 с. Це ≥3000 sample releases.
3. S2: той самий стенд, storage, ≥3615 с неперервного запуску та не менше 3600 с за часом MCU. Якщо симулятор повільніший за реальний час, продовжити до обох умов. 30 команд mark за schedule S2, 60 PIR-фронтів з HIGH≥150 мс та LOW≥150 мс; один фронт за хвилину. Уникати переповнення 128 slots, наприкінці stats/log та перевірка всіх CRC. Зіставити heap_arena на старті та щонайменше через 1 годину. Прискорений fake час host-тесту не зараховується.
4. S3: 315 с, стандартний стенд або явно задокументований event-clock fixture. `log` плюс періодична вибірка, UART statuses і справжній `erase CONFIRM` лише на тестовому архіві. Мінімум 30 фронтів PB1, які за VCD потрапили всередину PA7 HIGH; не зараховувати фронти поза інтервалом довгої операції. Якщо flash у Wokwi не реалізована або не має затримки, цей підсценарій позначити недоступним; HAL_Delay не додавати. Для перевірки впливу реального експорту окремо виміряти UART RX→відповідь і наявність backpressure.
5. UART boundary: schedules `uart-limit --rate 1/5/10/25/50/100/200` по 20 с кожний; 30 newline→response для кожної частоти. Зупинитися після доведеної втрати/перевищення 20 мс; зафіксувати останню успішну й першу невдалу частоти та counters. Не виводити поріг лише з baud-rate.
6. Fault: `imuaddr 0x69`, 10 с відсутності адреси, `imuaddr 0x68`, перевірити ADC/PIR/LED та повернення. Це адресний NACK. Адресний NACK не підтверджує поведінку при SDA low; для нього потрібна окрема цифрова ін'єкція несправності.
7. Startup: зовнішня мітка reset/початку VCD до першого коректного SAMPLE/ADC/WHO_AM_I. TIM3 починає рахунок після HAL clock init, тому сам не вимірює весь power-on interval. Запізнілий IMU повторити з fault/recovery.
8. Power cut: зберегти тестовий журнал, за PA7 HIGH перервати живлення/запис, повернути, зробити log і перевірити CRC та незмінність попередніх records. Зупинка Wokwi з повторним завантаженням HEX може очистити flash, тому сама по собі не доводить power-loss recovery. Перевірити фактичну семантику reset і збереження flash; висновок про recovery обмежений фактичною семантикою образу flash.

Команди підготовки schedules: `python3 tools/lab3_schedule.py S1 --output docs/metrics/lab3/scenarios/S1.json`, аналогічно S2/S3/fault. Збір UART: `python3 tools/serial_capture.py --port 4505 --seconds 310 --schedule docs/metrics/lab3/scenarios/S1.json --output docs/metrics/lab3/S1-uart.log`. З'єднати запис до reset, щоб зберегти банер; RFC2217 входить до конфігурації цього стенда. Аналіз: `python3 tools/lab3_analyze.py docs/metrics/lab3/S1-uart.log --output docs/metrics/lab3/S1.json`.

## Метрики та гіпотези

Поточні вимірювання та їхні обмеження наведено нижче; історичні файли metrics стосуються відповідних старих збірок. Гіпотези L1–L7 наведено в limitations.md. Зберігати ті самі правила release, визначення skipped/deadline, pinout і формат STAT. Для CPU порівнювати оцінку active time із тим самим обліком WFI/IRQ; RTOS idle hook матиме інший overhead, який потрібно оцінити окремо.


## Виміряна базова лінія 03.11.2026

ELF SHA-256 `db00e304227a9269662b8b430860d416a43ce1df68052fcace5620c868bea00c`; source SHA-256 `cd69576b8fb729c8f2e2dc52b71ae3bb41a01894ab1070dae8a56c312ab2aaac`. ARM FLASH 28544/28672 B, RAM 8048/12288 B. Wokwi, SysTick 1 мс, WFI вимкнено.

S1: 3051 sample, skipped=0, deadline_miss=0; незалежний VCD 3354 фронти, періоди 90,655–106,550 мс, медіана 99,998 мс. S2: безперервні ticks 3626000 мс, 36071 sample без пропусків і порушень, command skipped=64014 та deadline_miss=34; 61 PIR, 30 inspection. GDB знімок після експорту: 95/95 CRC і commit валідні. Stack watermark 876/2048 B, heap arena/used/free 1032/1032/0 B на початку й наприкінці; це не вимір корисного CPU load. Один IMU fault і ready після нього.

S3: 31 однозначна пара PIR–реакція під flash marker, 57,707–89,612 мс; після ~139 с відповіді на UART команди зникли за наявності доставлених RX байтів і подальших ticks. Причина не встановлена, кінцева статистика відсутня; окрема коротка діагностика не відтворила відмову. При SDA fault/звільненні в моделі IMU відновив ready. erase_ok=0 і старі записи залишилися попри HAL status 0. Recovery після перерваного запису, успішне стирання і ADC full-scale 4095 у Wokwi не підтверджені; 4095 перевірено host fake. Архіви UART, VCD, методики, GDB snapshot та розрахунки містяться у `Lab03/Work/final-20261103/{s1,s2,s3,fault,short-final,uart-diagnostic}/` власника; їх не замінюють старі результати. Це база для порівняння з ЛР4, а не доказ, що всі вимоги виконані.
