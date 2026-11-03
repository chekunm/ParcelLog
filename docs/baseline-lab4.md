# Базова лінія ParcelLog для ЛР4

Це підготовка повторюваних перевірок; FreeRTOS не реалізовано. Фінальна базова лінія ще не встановлена через відсутність запусків MCU/Wokwi ЛР3.

## Незмінні умови

STM32C031C6, HSI 48 МГц, UART 115200 8N1, I2C 100 кГц, той самий ELF/source manifest для S1–S3, -Os, TIM3 1 МГц, sample 100 мс. Форма B без фізичної плати. PA6/PA7/PA8 та LA канали див. captures/lab3/README.md. Засіб вимірювання не замінювати на host arrival timestamps. Зберегти build manifest, банер, UART і VCD разом.

## Відтворення

1. `make build`, `make test-linux`, `make coverage`; перевірити source-manifest і банер. Запуск Wokwi тільки у виділений інтерактивний слот.
2. S1: стандартний diagram, transit, PIR LOW, IMU 0/0/1g, LDR 500 lux; 310 с, stats на 305 с. Це ≥3000 sample releases.
3. S2: той самий стенд, storage, ≥3615 с неперервного запуску та не менше 3600 с за часом MCU. Якщо симулятор повільніший за реальний час, продовжити до обох умов. 30 команд mark за schedule S2, 60 PIR-фронтів з HIGH≥150 мс та LOW≥150 мс; один фронт за хвилину. Уникати переповнення 128 slots, наприкінці stats/log та перевірка всіх CRC. Зіставити heap_arena на старті та щонайменше через 1 годину. Прискорений fake час host-тесту не зараховується.
4. S3: 315 с, стандартний стенд або явно задокументований event-clock fixture. `log` плюс періодична вибірка, UART statuses і справжній `erase CONFIRM` лише на тестовому архіві. Мінімум 30 фронтів PB1, які за VCD потрапили всередину PA7 HIGH; не зараховувати фронти поза інтервалом довгої операції. Якщо flash у Wokwi не реалізована або не має затримки, цей підсценарій позначити недоступним; HAL_Delay не додавати. Для перевірки впливу реального експорту окремо виміряти UART RX→відповідь і наявність backpressure.
5. UART boundary: schedules `uart-limit --rate 1/5/10/25/50/100/200` по 20 с кожний; 30 newline→response для кожної частоти. Зупинитися після доведеної втрати/перевищення 20 мс; зафіксувати останню успішну й першу невдалу частоти та counters. Не виводити поріг лише з baud-rate.
6. Fault: `imuaddr 0x69`, 10 с відсутності адреси, `imuaddr 0x68`, перевірити ADC/PIR/LED та повернення. Це адресний NACK. Утримання SDA і фізичне від'єднання є окремими тестами, не підтверджуються NACK.
7. Startup: зовнішня мітка reset/початку VCD до першого коректного SAMPLE/ADC/WHO_AM_I. TIM3 починає рахунок після HAL clock init, тому сам не вимірює весь power-on interval. Запізнілий IMU повторити з fault/recovery.
8. Power cut: зберегти тестовий журнал, за PA7 HIGH перервати живлення/запис, повернути, зробити log і перевірити CRC та незмінність попередніх records. Зупинка Wokwi з повторним завантаженням HEX може очистити flash, тому сама по собі не доводить power-loss recovery. Перевірити фактичну семантику reset і збереження flash; за відсутності підтримки потрібен фізичний стенд.

Команди підготовки schedules: `python3 tools/lab3_schedule.py S1 --output docs/metrics/lab3/scenarios/S1.json`, аналогічно S2/S3/fault. Збір UART: `python3 tools/serial_capture.py --port 4505 --seconds 310 --schedule docs/metrics/lab3/scenarios/S1.json --output docs/metrics/lab3/S1-uart.log`. З'єднати запис до reset, щоб зберегти банер; старе з'єднання RFC2217 іншого проєкту не використовувати. Аналіз: `python3 tools/lab3_analyze.py docs/metrics/lab3/S1-uart.log --output docs/metrics/lab3/S1.json`.

## Метрики та гіпотези

Підтверджена пам'ять буде у metrics/lab3/memory.log, host результати в host-scenarios.log. Усі часові клітинки S1/S2/S3 у metrics/lab3/README.md поки порожні. Гіпотези L1–L7 наведено в limitations.md. Зберігати ті самі правила release, визначення skipped/deadline, pinout і формат STAT. Для CPU порівнювати оцінку active time із тим самим обліком WFI/IRQ; RTOS idle hook матиме інший overhead, який потрібно оцінити окремо.
