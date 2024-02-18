# LVGL TEMPLATE
Basic template for ili9341 driver and esp32.

## About ESP32

* board: ESP32-WROOM
* flash size: 4MB
* esp-idf: v4.2.1

## About LCD TFT

* LCD driver: ILI9341
* LCD module: 2.8 TFT SPI 240x320 v1.2
* Touch driver: XPT2046

<img src="assets/lcd.PNG" width="75%" > </img>

## About LVGL

* LVGL : v8.0.0-194-gd79ca388
* LVGL commit : d79ca38
* LVGL esp32 drivers commit: a68ce89 

## Wiring

| module TFT   | ESP32    |
| ---          | ---      |
| T IRQ        |  gpio 25 |
| T DO  (MISO) |  gpio 19 |
| T DIN (MOSI) |  gpio 23 |
| T CS         |  gpio 5  |
| T CLK        |  gpio 18 |
| SDO (MISO)   |  NC      |
| LED          |  gpio 27 |
| SCK          |  gpio 14 |
| SDI (MOSI)   |  gpio 13 |
| DC           |  gpio 2  |
| RESET        |  gpio 4  |
| CS           |  gpio 15 |
| GND          |  GND     |
| VCC (5v)     |  NC      |

* Used 10cm length wires
