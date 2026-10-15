# ParcelLog: власні записи

Джерело acceptance.vcd.gz, SHA256 повного VCD у manifest.json. Запис отримано 15.10.2026, кінцевий час 130,181600650 с. Всі вісім каналів походять з одного запуску Wokwi. Вікна після запису обрізані із збереженням первинного offset та hash у .origin.json. Параметри SR: I2C/UART/ADC detail 20 МГц, довгі GPIO 1 МГц. Цифровий маркер не є аналоговим вимірюванням.

WHO_AM_I: 0x68, pointer0x75. Burst14 bytes із0x3B. i2c-manual-address має PVS без decoder. i2c-nack містить адресу0x69 та slave NACK, не master NACK завершення read. UART 115200 8N1. PIR PB1, MODE PB0, ADC_MARK PA4, LED PA5. Декоди отримані sigrok-cli, screenshots власного PulseView.

Для відтворення selftest запускає ID/config; imuaddr 0x69 створює NACK, imuaddr 0x68 повертає нормальну адресу. Вхідні дані та висновки наведено в [звіті](../lab2/report.pdf). Використано існуючу Community License Wokwi, trial не потрібен.
