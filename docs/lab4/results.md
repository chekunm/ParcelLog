# ParcelLog — ЛР4

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi; STM32C031C6, 48 МГц. [Задачі й конфігурація](../rtos-design.md).

Порівняльна bare-metal база та RTOS заморожені в `../evidence/lab4/{lab3,rtos}/`: джерела, ELF/HEX/BIN, карти й manifests. `identities.json` зберігає їх SHA-256. Для відтворення архівів зберегти generated build_version.h і виконати `make -C firmware BUILD_DIR=build-reproduction all` у відповідному Docker-образі. Нова збірка не замінює виміряний ELF.

Обидва S2 14.11.2026 дали по 3625000 мс MCU, 60 PIR, 30 inspection і 94 CRC-valid записи без пошкоджень. FR-11/12/13 залишилися failed: невдалий erase, UART до 43 мс при цілі 20 мс та недостатня повна серія S3 під flash. Сирі результати, входи й розрахунки — measurements/*.tar.gz; повна методика, кількісне порівняння й обмеження — у PDF цієї папки.
