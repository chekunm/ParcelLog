# ParcelLog

Чекун Микола Олексійович, ІО-33. Спостереження за посилкою: MPU6050, LDR, PIR, transit/storage та CRC-журнал.

| Робота | Реалізація | Результати й звіт |
|---|---|---|
| ЛР2 | GPIO/I2C/ADC та окремі драйвери | [Матеріали](docs/lab2/) |
| ЛР3 | Bare-metal super loop, гілка feature/lab-3 | [Результати та PDF](docs/lab3/) |
| ЛР4 | FreeRTOS, гілка feature/lab4-freertos | [Результати та PDF](docs/lab4/) |

[PRD](docs/PRD.md) · [Схема задач](docs/rtos-design.md) · [Архітектура](docs/architecture.md).

`make test` — host-тести; `make build` — ARM-збірка через Docker в окремому firmware/build-lab4-work. Звичайний wokwi.toml вибирає збережену RTOS-прошивку. Виміряні джерела/ELF/HEX та сирі дані збережено в docs/evidence; [інструкція відтворення й негативні результати](docs/lab4/results.md). Нова збірка не успадковує експерименти автоматично.
