# ParcelLog: задачі FreeRTOS

Плата Nucleo-C031C6, Cortex-M0+, 48 МГц. Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi.
Офіційне ядро FreeRTOS-Kernel V11.1.0, MIT, джерело й SHA архіву у firmware/Middlewares/FreeRTOS/VERSION.txt. ARMv6-M port GCC/ARM_CM0, MPU вимкнено. Порт використовує PRIMASK, тому твердження про BASEPRI/configMAX_SYSCALL_INTERRUPT_PRIORITY з прикладу Cortex-M4 сюди не переносяться. USART2 і EXTI можуть викликати тільки FromISR API; друк та правила посилки залишено задачам.

| Задача | Пріоритет | Запуск | Stack reserve, B | Обґрунтування |
|---|---:|---|---:|---|
| pir | 3 | Queue 8 edge snapshots з EXTI | 512 | FR-06/13, короткі фронти не залежать від 100-мс polling |
| sample | 2 | 100 мс, delay-until | 896 | FR-01–06/09/13; фазові releases, skipped і deadline збережені |
| console | 4 | кожний tick, до 128 bytes/1 command | 1024 | FR-07/12, 2-мс deadline parser і ≤20мс newline→enqueue — найжорсткіші часові цілі |
| service | 1 | кожний tick; save/export, status1с, LED500мс | 1024 | FR-08/10/11, низькопріоритетний супровід |
| IDLE | 0 | коли інші Blocked | 320 | runtime idle, без WFI відповідно до відомої межі моделі |

Задачі створюються лише на старті через heap_4; configTOTAL_HEAP_SIZE=5120 B. Динамічного створення/видалення задач під час роботи немає. Stack reserve входить у heap ОС, його не треба додавати до .bss вдруге. MSP має окремі 1024 B. C malloc програмою не використовується; integer-only форматувальник не створює FILE/heap і зберігає UART-рядки ЛР3.

Схема: EXTI PB1 → queue {timestamp,physical mode} → pir; SysTick → periodic releases → sample/console/service; state mutex → спільні rules/sample/journal/FIFO/formatter; USART IRQ → RX256+timestamp256 → console; formatter → TX2048 → USART IRQ.

М’ютекс із успадкуванням пріоритету зберігає атомарність сукупного стану та рядків. Це свідомо консервативний перший перенос: під час синхронного I2C/flash володілець тримає state mutex, тому сама поява високопріоритетної задачі не гарантує коротку затримку. Це обмеження треба виміряти у S3, а не приховувати. Queue overflow PIR має окремий лічильник у `tasks`; переповнення існуючого журналу та UART зберігають старі counters. Знімок режиму ISR не є доказом фізичного доступу до упаковки.

FR-02/03/04, пороги й одиниці не змінено. PIR у sample не створює дубль уже прийнятого ISR-фронту. Already-HIGH при вході storage не генерує нового rising IRQ. Pending edges, прийняті в transit, ігноруються. Для спільного стану одночасна зміна фізичного режиму та PIR уточнюється цифровим сценарієм; час mode polling залишається 100 мс.

SysTick спільний для HAL і ОС: wrapper збільшує HAL tick у мілісекундах (1 при 1000Hz, 10 при 100Hz), потім викликає port handler. До старту scheduler HAL працює з початковим tick1мс. Власний weak-hook порту перемикає частоту й скидає pending tick до початку задач; console ініціалізує runtime після цього перемикання. ISR не викликають HAL timeout API. В обох нових порівняльних версіях instrument_now() читає HAL tick + SysTick VAL і pending bit під PRIMASK, з кроком 1 мкс при 48 МГц. 32-бітний timestamp обертається через приблизно 71,6 хв; різниці unsigned і 64-бітна сума тримають довші інтервали. Кілька coalesced ticks під час тривалого stall метод не відновлює; незалежний GPIO/VCD потрібний для перевірки. DWT не застосовується.

`stats` зберігає METRICS/STAT поля; truthful clock label дорівнює SysTick_us. Нова команда `tasks` друкує TaskStatus_t, min-free stack у байтах, runtime, heap free/min, tick handler cost і trace. Тривалість task wall span включає витіснення; runtime counter оцінює зайнятість task context з кроком 1 мкс і не відокремлює усі IRQ. Tick cost читає VAL безпосередньо, switch-out→switch-in квантується по 48 номінальних cycles і не є повною вартістю notify-to-task; для останньої передбачено окремий 30-event experiment.

Варіанти §7.2 задає LAB4_EXPERIMENT, кожний має окремий каталог build і вхідні ELF/HEX. Синтетичне CPU-навантаження інверсії/голодування не видається за запис flash продукту. Продуктовий S3 виконує справжні HAL program/erase й експорт.
