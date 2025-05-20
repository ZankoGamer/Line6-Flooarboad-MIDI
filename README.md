# Line6 Floorboard ESP32-S3 Wireless MIDI Controller

Transform your Line6 Floorboard into a BLE MIDI foot controller with just an ESP32‑S3 and a few solder joints.
This firmware also runs on any custom footswitch rig, you just have to provide your own switches and LEDs with proper resistors.

## Hardware Modifications

### Step 1: Footswitch Board

1. **Remove resistors** at the highlighted locations and solder wires to the points indicated. These wires become your switch inputs.
   ![Footswitch board](hw%20modifications/line6%20board%20switches.jpg)
2. **Connect each switch wire** to ESP32‑S3 GPIO pins of your choice.
3. **LEDs**: Use continuity/diode mode on your multimeter between GND and each pin of the on‑board LED connector to identify LED pins. If an LED doesn’t light up, probe between GND and the solder pad on the board side of its resistor (not directly at the LED).
   *Each LED already has a current‑limiting resistor, so you only need to wire power, GPIO, and GND.*

### Step 2: Expression Pedals & Wah

1. **Solder wires** to the pedal board’s potentiometer output, VCC, and GND. Also tap the onboard connector for the wah‑LED pin.
   ![Expression board](hw%20modifications/expression%20board%20front.png)
2. **Connect** all pedal wires to the ESP32‑S3’s pins.

The completed board should look something like this. You can found other closeups in the "hw modfications" folder.
![](hw%20modifications/1747751318078.jpg)

## Software Setup

1. **Arduino IDE**: Install and select the ESP32‑S3 board.
2. **BLE‑MIDI Library**: Copy the included, NimBLE‑compatible library into your Arduino `libraries/` folder.
3. **loopMIDI**: Download and create a virtual MIDI port: [https://www.tobias-erichsen.de/software/loopmidi.html](https://www.tobias-erichsen.de/software/loopmidi.html)
4. **BLE‑MIDI Connect**: Install from Microsoft Store (free unlimited version; paid supports dev): [https://apps.microsoft.com/detail/9NVMLZTTWWVL?hl=neutral\&gl=IT](https://apps.microsoft.com/detail/9NVMLZTTWWVL?hl=neutral&gl=IT)

## Firmware & Calibration

* Upload `HACKEDLine6Floorboard.ino` to the board.
* Sends **undefined CC messages** (perfect for MIDI‑learn).
* Pedal heel = min, tip = max; invert indices in smoothing if desired.

### Calibration

1. In `HACKEDLine6Floorboard.ino`, enable `CALIBRATION_MODE`.
2. Set `analogRead(VOLUME_PEDAL_PIN)` for the pedal you’re calibrating.
3. Power on with pedal at heel; note printed value.
4. Restart with toe position; note value.
5. Enter those min/max into the boundary definitions and disable `CALIBRATION_MODE`.

## Usage

1. Pair the board in BLE‑MIDI Connect (enable “Show all devices”).
2. In loopMIDI, select your new port as the output.
3. Open your DAW, choose the loopMIDI port as MIDI input. That's all.
![](hw%20modifications/Immagine%202025-05-20%20163453.png)

