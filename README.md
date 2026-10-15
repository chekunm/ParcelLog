# ParcelLog

Прототип спостереження за посилкою на STM32C031C6: MPU6050, LDR, PIR та фізичний перемикач transit/storage. Події не встановлюють особу людини або факт відкриття упаковки. UART-журнал не переживає reset.

[Звіт ЛР2](docs/lab2/report.pdf) · [PRD](docs/PRD.md) · [Pinout](docs/pinout.md) · [Архітектура](docs/architecture.md) · [Власні захвати](docs/captures/README.md)

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

```sh
make build
make test
make test-linux
make coverage
```

У VS Code відкрити корінь проєкту та виконати Wokwi: Start Simulator. Початково MPU X/Y=0, Z=1 g, gyro=0; LDR 500 lux, PIR LOW. PB0=LOW означає transit, HIGH означає storage. PIR підключено до PB1. UART 115200 8N1 приймає `status`, `selftest`, `imuaddr 0x69`, `imuaddr 0x68`. Немає команд calibrate, ack або програмного вибору mode. ADC marker PA4, LED heartbeat PA5.

App і Devices не включають HAL. Platform реалізує hardware I2C AUTOEND із deadline 10 ms, ADC calibration/retry та UART IRQ queues. Старий GPIO bit-bang з ЛР1 замінено апаратним I2C; вихідні HAL/CMSIS з commit 2b82b2a збережено, SHA256 перевіряються автоматично.

Власний запис 15.10.2026 підтвердив WHO_AM_I 0x68, SCL 100 кГц, 100-мс опитування IMU/ADC, контрольований NACK і відновлення. Три host suites та ASan/UBSan пройшли. Coverage App+Devices 100% рядків,96,43% гілок; Board/HAL не включені, runtime fake перевіряється окремо. Живлення й аналогова точність фізичного стенда не вимірювались.

ЛР2 у feature/lab-2; pull request до main створюється окремо.
