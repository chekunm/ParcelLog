# ЛР2 ParcelLog

Чекун Микола Олексійович, ІО-33. [Фінальний звіт](lab2/report.pdf). Усі вимірювання отримано 15.10.2026 на власному Wokwi стенді.

MPU6050, LDR та PIR працюють одночасно. PB0 обирає transit/storage, PB1 читає PIR. Зіставлено pinout, задокументовано 3,3 V логіку та межі моделі живлення. Немає камери, SPI-flash, PWM або постійного журналу.

Запис WHO_AM_I підтвердив 0x68, burst у спокої дає Z=16384 raw та 1000 mg, temp=24001 m°C. SCL 100 кГц; IMU близько 100 мс, ADC 99,99565 мс, software marker 65,225 мкс. На 47205 мс виникли shock/light_change/nearby_motion, на 47405 мс tilt. Помилка адреси 0x69 дала slave NACK, інші канали працювали; 0x68 відновив IMU. Власні VCD/SR/PVS, декоди, offsets і SHA256 у docs/captures.

Виправлено серію нахилу через invalid або проміжну вибірку. HAL-free App/Devices перевіряються host tests. Coverage 100% рядків та 96,43% гілок стосується тільки цих шарів. Runtime перевіряється fake board/clock/UART окремо. Фізичне обладнання та метрологію не перевірено.
