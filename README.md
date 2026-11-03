# ParcelLog

Чекун Микола Олексійович, ІО-33. Спостереження за посилкою: MPU6050, LDR, PIR, transit/storage та CRC-журнал.

ЛР3 виконується без ОС. [Результати ЛР3](docs/lab3/results.md), PDF у docs/lab3, [архітектура](docs/architecture.md), [PRD](docs/PRD.md).

`make test` — host-тести; `make build` — ARM-збірка через Docker. Виміряні ELF/HEX та джерела збережено окремо в docs/evidence/lab3; нова збірка не успадковує старі результати автоматично. У Wokwi відкрити wokwi.toml. [Інструменти](tools/HEADLESS-TOOLS.md).
