# A Split Keyboard, Built from Scratch

## Overview

This is a custom split mechanical keyboard that I designed with two ESP32-S3 microcontrollers, one for each half. I added a few extra features different from a normal keyboard, including 1.8-inch TFT displays, rotary encoders, RFID/NFC-based locking, and adjustable per-key backlighting.
This keyboard can be used as wired keybaord as well as the wireless keybaord with the 2 bluetooth profile 

![Fully Assembled Keyboard](images/kbd-final.png)


##  Collaboration & Attribution (Team Project)

This project was built in collaboration with my friend **Mr. Chronos** ([Chronos-03/Chronos-keybaord](https://github.com/Chronos-03/Chronos-keybaord)).as I was learning several things from scratch, we worked together over meet sessions. our files look very similar beause they were made side by side in real time. I created my own project files step-by-step alongside him and mutually debugged issues.
you'll see that my schematics and pcb are exactly same as Mr. Chronos, so i wanna clarify that my schematic was made by me only and it looks same bcz we did it side by side together on our own projects, and about the pcb, we both made our own first(as recorded in lapse) but i chose his pcb as the final one as it was better for manufacturing....but i hope we get the time recorded on the lapse for that.

###  Breakdown of Contributions (What I Did vs. What Chronos Did)

**What I Did Myself (Time Claimed):**
Drew the complete schematic in KiCad myself while following along in real time, verifying component connections, and helping spot/fix schematic errors during our sessions.Placed the components, routed the PCB tracks on my board, and resolved the DRC/ERC errors with his help. Built the 3D case step-by-step while learning CAD modeling from him. I also independently modified and fine-tuned my case design by adding rounded corners (fillets) to give it a smoother, distinct aesthetic compared to his.

**What Mr. Chronos Did (Time NOT Claimed):**
Guided me through the schematic design, PCB routing rules, error resolution, and 3D CAD workflow via screen share.The PCB panelization / mouse bites and shared fabrication prep were done directly by Mr. Chronos. **I have not claimed any time for this part.**

## Why did i make this
I just wanted to make something cool which i myself yould love to use in a long run.And this idea came randomly, while discussing projects with my friend Mr. Chronos.We both liked the idea, so we decided to stick with it and gradually started adding more features to make it our own.

## Key Features

### Split Mechanical Layout

the keyboard is in two separate halves, with each half having its own ESP32-S3 microcontroller. Each side has 21 Cherry MX switches, giving the keyboard a total of 42 keys. I also included individual backlighting for the keys.
![Full Keyboard Layout](images/keyboard-layout.png)

### Rotary Encoder Controls

EC11 rotary encoders helps in  easily adjusting settings like volume and screen brightness.

![Rotary Encoder Schematic](images/rotary-encoders.png)

### TFT Display

Two 1.8-inch TFT displays show information like battery level and connection status. They can also play animations when the keyboard is not in use.

![Display Schematic](images/display.png)

### RFID Integration

integrated an RC522 RFID reader into each half. This allows the keyboard to read RFID cards and NFC tags. I also added the option to use this feature as a keyboard lock or to trigger PC unlocking through a supported NFC tag or phone.

![RFID Schematic](images/rfid.png)

### Rechargeable Power

Each half has a rechargeable 10,000 mAh LiPo battery and a built-in BMS to power all its components. this can provide long battery life when using Bluetooth mode.

![Power Management Schematic](images/power-management.png)

### Per-Key Backlighting

White 0603 SMD LEDs are use to work as the backlight like any professional keybaord. its intensity can be controlled  with the same rotatory encoder.

![LED Schematic](images/resistors-leds.png)


## Schematic Design

### Key Switch Matrix

The switch matrix connects the mechanical switches to the microcontroller. diodes prevent unwanted signals when multiple keys are pressed at the same time.

![Key Switch Matrix](images/key-matrix.png)

### Switches and Diodes

Each mechanical switch registers an individual key press, while the 1N4148WS diodes help prevent unwanted signal interference when multiple keys are pressed together.

### Resistors and LEDs

Resistors support the circuits and help in its proper working, while white SMD LEDs provide backlighting for each key.

![Resistors and LEDs](images/resistors-leds.png)

### Microcontroller

The ESP32-S3-WROOM-1-N4R8 acts as the main controller for each half.

![ESP32-S3 Schematic](images/esp32-s3.png)

### Rotary Encoders

The EC11 encoders can be pressed or rotated to access additional keyboard controls.

![Rotary Encoder Schematic](images/rotary-encoders.png)

### Display Circuit

The TFT displays are connected to the ESP32-S3 so they can show battery information, connection status, and other keyboard details.

![Display Schematic](images/display.png)

### RFID Circuit

The RC522 modules are connected to the ESP32-S3 to read RFID cards and NFC tags.

![RFID Schematic](images/rfid.png)

### Power Management

For the power section, TP4056 charging ICs, ME6211 voltage regulators and AO3400A MOSFETs are used. I also added resettable PTC protection.

![Power Management](images/power-management.png)

### USB-C Connectivity

The USB-C ports are used for charging, uploading firmware, and connecting the keyboard using a wired connection.

![USB-C Schematic](images/usb-c.png)


## PCB Design

The schematic and PCB layout were designed in KiCad, and the boards are manufactured by Zbotic. The PCB contains the keyboard matrix, controller connections, peripheral interfaces, LED backlighting, and power circuits.

![PCB Layout](images/pcb-layout.png)


## CAD Design

The keyboard case was designed in Fusion 360 to fit all the components, including the custom PCB, switches, displays, rotary encoders, and other internal electronics.

### Enclosure

![Keyboard Enclosure](images/enclosure.png)

### Complete Assembly

![Complete CAD Assembly](images/cad-assembly.png)


## Firmware Status

I wrote the firmware before the physical hardware was assembled, so I haven't been able to test it on the actual PCB yet. Once the boards are ready, I'll flash them and check each component individually. I'll make changes wherever needed and update the code based on how everything performs on the actual hardware.

**AI disclosure** : as zmk and qmk have their own limitations, so that's why I took help from AI to make custom firmware which can run all the components flawlessly, like display, encoder, RFID, and backlight. 

## Display, Backlight, and NFC Features

Some additional features that I added to this project:
* **Display:** A 1.8-inch SPI TFT display that shows battery status, backlight brightness, the active layer, and NFC tag UIDs.
* **Backlight:** 0603 SMD LEDs with PWM brightness control through the rotary encoder and wireless syncing.
* **NFC:** An RC522 NFC module connected through SPI for scanning NFC tags.

Getting the display, backlight, and NFC features working alongside the wireless keyboard took quite a bit of work. I had to make changes to the firmware to get everything working together. The firmware is ready for testing, but the actual on-board checks and final testing will be done once the physical hardware is assembled.


## AI Declaration
I completed the hardware design, schematic capture, PCB layout, and 3D CAD modeling myself alongside guidance from **Mr. Chronos**. For the firmware, I wrote the complete C++ codebase myself and used AI only for a final review, debugging, and checking the code before flashing.

## Bill of Materials (BOM)

| Item | Qty | Supplier | Cost/item | Total Cost | Link |
| :--- | :--- | :--- | :--- | :--- | :--- |
| ESP32-S3-WROOM-1 Module | 2 | Robu.in | $6.03 | $12.06 | [Buy](https://robu.in/product/espressif-esp32-s3-wroom-1-n16r8-module/) |
| NFC RFID Reader | 2 | IndianHobbyCenter | $1.47 | $2.94 | [Buy](https://www.indianhobbycenter.com/products/rc522-mini-rfid-reader-writer-spi-s50-with-rfid-card-and-tag) |
| PCB | 1 | Zbotic | $45.00 | $45.00 | [Buy](https://zbotic.in/) |
| 1.8" TFT Display | 2 | Robu.in | $3.28 | $6.56 | [Buy](https://robu.in/product/62060/) |
| Cherry MX2A Mechanical Switches | 42 | meckeys | $0.41 | $17.22 | [Buy](https://meckeys.com/shop/accessories/keyboard-accessories/key-switches/cherry-mx2a-switch/) |
| 1N4148WS SMD Diodes (SOD-323) | 42 | surnom.com | $0.01 | $0.42 | [Buy](https://www.sunrom.com/p/1n4148---1n4148ws---t4---smd---sod323-0805) |
| EC11 Rotary Encoder | 2 | robu.in | $0.61 | $1.22 | [Buy](https://robu.in/product/hongyan-ec11h-7ce15p1zd15f7-rotary-encoder-with-push-button-switch-vertical-plug-in-5-pin-360-degree/) |
| 0603 SMD Backlight LEDs | 42 | ktron.in | $0.01 | $0.42 | [Buy](https://www.ktron.in/product/smd-white-led-0603-package/) |
| AO3400A N-Channel MOSFET | 2 | robu.in | $0.06 | $0.12 | [Buy](https://robu.in/product/1-month-warranty-414/) |
| SMD 16-pin connector | 4 | ktron.in | $0.06 | $0.24 | [Buy](https://www.ktron.in/product/16-pin-usb-type-c-connector-smd/) |
| 10,000 mAh LiPo Battery | 2 | makerbazar.in | $4.23 | $8.46 | [Buy](https://makerbazar.in/products/single-cell-3-7v-10000mah-lipo-battery) |
| TP4056 IC | 2 | ktron.in | $0.05 | $0.10 | [Buy](https://www.ktron.in/product/tp4056-ic/) |
| ME6211A33M3G IC | 2 | Robu.in | $0.08 | $0.16 | [Buy](https://robu.in/product/me6211c33m5g-n-microne-500ma-fixed-3-3v-positive-electrode-6v-sot-23-5-voltage-regulators-linear-low-drop-out-ldo-regulators-rohs/) |
| SMD1206P050TF/13.2 (PTC) | 2 | Robu.in | $0.10 | $0.20 | [Buy](https://robu.in/product/smd1206-050c-6v-bnstar-6v-40a-500ma-1a-1206-resettable-rohs/) |
| SMD Push Switches | 4 | Robu.in | $0.05 | $0.20 | [Buy](https://robu.in/product/tsa043a2026a-bzcn-3x4x2-5-50ma-round-button-50gf-12v-smd-tactile-switches-rohs/) |
| Slide Switch | 2 | Robu.in | $0.03 | $0.06 | [Buy](https://robu.in/product/1-month-warranty-254/) |
| 100Ω Resistor | 42 | ktron.in | $0.01 | $0.42 | [Buy](https://www.ktron.in/product/resistor-100-ohms-1-1-8w-smd-0805/) |
| 10kΩ Resistor | 12 | ktron.in | $0.01 | $0.12 | [Buy](https://www.ktron.in/product/resistor-10k-ohms-1-1-8w-smd-0805/) |
| 56kΩ Resistor | 4 | ktron.in | $0.01 | $0.04 | [Buy](https://www.ktron.in/product/resistor-56k-ohms-1-1-8w-smd-0805/) |
| 5.1kΩ Resistor | 4 | ktron.in | $0.01 | $0.04 | [Buy](https://www.ktron.in/product/resistor-5k1-ohms-5-1-8w-smd-0805/) |
| 1.2kΩ Resistor | 2 | ktron.in | $0.01 | $0.02 | [Buy](https://www.ktron.in/product/resistor-1k2-ohms-1-1-8w-smd-0805/) |
| 10µF Capacitor | 10 | ktron.in | $0.08 | $0.80 | [Buy](https://www.ktron.in/product/10uf-50v-multi-layered-ceramic-capacitor-mlcc-x5r-smd-0805-yageo-cc0805kkx5r9bb106/) |
| 1µF Capacitor | 2 | ktron.in | $0.09 | $0.18 | [Buy](https://www.ktron.in/product/1uf-50v-multi-layered-ceramic-capacitor-yageo/) |
| 100nF Capacitor | 8 | ktron.in | $0.01 | $0.08 | [Buy](https://www.ktron.in/product/100nf-25v-multi-layered-ceramic-capacitor/) |
| Estimated total shipping | 1 | | | $0.00 | |
| **Grand Total** | | | | **$97.08** | |


## Tools & Technologies

- **PCB Design:** KiCad
- **CAD Modelling:** Fusion 360
- **Firmware:** Cpp, ESP-IDF, FreeRTOS
- **Microcontroller:** ESP32-S3
- **PCB Manufacturing:** Zbotic