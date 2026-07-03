# ESP32 Filament Scale - Build Guide

## Overview
A simple, no-frills filament scale that tells you exactly how much filament is left on a spool. Place any spool flat on the top plate and it instantly shows three net weights simultaneously - one for each common spool type. No buttons, no menus, no app, no WiFi. Just power it on and weigh.

If you're ever in doubt or confused, just ask you favorite Ai to help you build it. 

---

## Parts List

| Part | Notes |
|---|---|
| ESP32 Development Board (USB-C) | Elegoo or similar with CP2102 chip |
| 5kg bar load cell | Comes with 4 wires (red, black, green, white) |
| HX711 ADC module | Get one with E+/E-/A+/A- labeled |
| 0.96" SSD1306 OLED display | Yellow/blue two-color I2C version, 128x64 |
| M4x20mm screws | 4 total, for mounting load cell to printed plates |
| M2x6mm screws + nuts + washers | For mounting the OLED to the case |
| Dupont wires | Female to female |
| Heat shrink tubing | For the 3.3V wire splice |
| Electrical tape | For securing wire bundles and connections |
| USB-C cable with inline rocker switch | For easy power on/off (recommended) |

Total parts cost is typically under $20 when buying component packs.

---

## Tools Needed

- Soldering iron and solder
- Wire cutters and strippers
- Side cutters for trimming header pins
- USB-C cable for programming

---

## Step 1 - Solder Header Pins onto HX711

The HX711 ships with loose header pins that need to be soldered in before anything else.

- Cut the strip with side cutters: 6 pins for the load cell side (E+, E-, A-, A+, B-, B+) and 4 pins for the ESP32 side (GND, DT, SCK, VCC)
- Insert pins with the **short side into the board**, long side sticking up
- Solder from the bottom of the board
- B- and B+ will go unused

---

## Step 2 - Mount Load Cell to Printed Plates

- The load cell has 4 threaded holes, 2 on each end
- Bottom plate bolts to the **fixed end** of the load cell
- Top plate bolts to the **load end** (the end that deflects downward under weight)
- Use M4x20mm screws directly into the load cell threaded holes, no heat inserts needed

> **Critical:** the two plates must only connect via the load cell. If they touch anywhere else the load cell cannot deflect and readings will be wrong.

---

## Step 3 - Wiring

### Load Cell to HX711

| Load Cell Wire | HX711 Pin |
|---|---|
| Red | E+ |
| Black | E- |
| Green | A+ |
| White | A- |

Leave B+ and B- empty.

### HX711 to ESP32

| HX711 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| DT | GPIO 4 |
| SCK | GPIO 5 |

### OLED to ESP32

| OLED Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### The 3.3V Splice

Both the HX711 and OLED need 3.3V but the ESP32 only has one 3.3V pin. Splice them together:

- Leave the female dupont connectors intact on both the HX711 VCC and OLED VCC wires
- Cut the wires somewhere in the middle and strip the ends
- Twist all three together with a third wire running to the ESP32 3.3V pin
- Solder the splice and cover with heat shrink
- GND is not an issue as most ESP32 boards have two GND pins

### Power Switch (Recommended)

Add a USB-C cable with an inline rocker switch between your power source and the ESP32. The scale needs to be powered off and back on occasionally to re-zero, especially if it drifts after sitting for a while. An inline switch makes this easy without unplugging anything.

---

## Step 4 - Important I2C Address Note

The OLED board has **0x78 or 0x7A printed on it - ignore it.** The correct address in code is **0x3C**. Using the printed address will make it appear the display is dead.

---

## Step 5 - Install Libraries in Arduino IDE

Install these via Tools > Manage Libraries:

- **HX711 by Rob Tillaart** (specifically Tillaart, not the Bogde version)
- **Adafruit SSD1306**
- **Adafruit GFX Library**

Add the ESP32 board package if you haven't already. Go to File > Preferences and add this URL under Additional Board Manager URLs:https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
Then go to Tools > Board > Board Manager and install **esp32 by Espressif**. Select your board as **ESP32 Dev Module**.

---

## Step 6 - Flash the Code

Open the provided sketch and flash it to your ESP32. Before calibrating, note these values at the top of the sketch:

```cpp
#define CALIBRATION_FACTOR  370.0f

const float TARE_BAMBU_G      = 255.0f;
const float TARE_CARDBOARD_G  = 150.0f;
const float TARE_PLASTIC_G    = 165.0f;
```

---

## Step 7 - Calibration

The calibration factor of 370.0 is a starting point. Your specific load cell will likely need a small adjustment.

1. Open Serial Monitor at 115200 baud after flashing
2. Place a known weight on the scale (a factory-sealed 1kg spool or a kitchen-scale verified object works well)
3. Watch the gross weight output in Serial Monitor
4. Adjust `CALIBRATION_FACTOR` up or down until the reading matches your known weight
5. Reflash with the corrected value
6. Power cycle the scale with nothing on it to zero it out

---

## Tare Weights Explained

The three hardcoded tare values cover the most common spool types you will encounter.

**Bambu Lab plastic spools - 255g**
Bambu spools are injection molded plastic and very consistent across their lineup. 255g is accurate for the vast majority of Bambu spools regardless of filament type.

**PolyMaker and similar cardboard core spools - 150g**
Cardboard core spools have become increasingly common as a more eco-friendly option. 150g is a reliable middle-ground figure for this style across most brands.

**Generic lightweight plastic spools - 165g**
Brands like 3D Fuel and similar budget or specialty filament makers use a thinner, lighter plastic spool that comes in around 165g. This is lighter than Bambu-style spools so it gets its own row.

If your spools do not match these exactly, weigh a few empty ones on a kitchen scale and update the values in the code. Being off by 10-20g is fine for the intended purpose of knowing whether you have enough filament to finish a print.

---

## How It Works

Place any spool flat on the top plate. The scale reads gross weight, subtracts all three tare values simultaneously, and displays all three net filament weights at once.

Actual        842g
Bambu         587g
Cardbd        692g
Plastic       677g

The yellow band at the top shows the raw gross weight. The blue zone below shows all three net weights. Negative results display as `---` meaning the spool is lighter than that tare value, which just means it is not that spool type.

The display sleeps after 3 minutes of inactivity and wakes automatically when a spool is placed on the scale.

---

## Mounting the OLED

The OLED mounts to the case using M2x6mm screws with nuts and washers. The washers matter here as the OLED PCB mounting holes are small and the screw head can pull through without them.


