#include <ezButton.h>
#include <BLEMidi.h>
#include <Arduino.h>
#include <esp32-hal-cpu.h>

// Uncomment to enable calibration routine for debugging
// #define ENABLE_CALIBRATION

// Constants
constexpr uint8_t MIDI_CHANNEL    = 15;    // 0-15 => MIDI channels 1-16, here channel 16
constexpr uint8_t CC_MODULATION   = 1;     // CC number for modulation (wah)
constexpr uint8_t CC_VOLUME       = 7;     // CC number for volume control

// Hardware pin definitions
//The buttons and LED correspond to
//TUNER - CHANNEL SEL UP - CHANNELSEL DOWN - A - B - C - UP - DOWN - D - WAH
//TUNER - CHANNEL SEL UP                   - A - B - C - UP - DOWN - D - WAH
constexpr int BUTTON_NUM             = 9;
constexpr int LED_NUM                = 9;
const int buttonPins[BUTTON_NUM]     = {4, 5, 6, 7, 15, 16, 17, 18, 8};
const int ledPins[LED_NUM]           = {1, 36, 38, 2, 21, 48, 45, 47, 37};
constexpr int LED_CONN_PIN           = 35;  // LED for connection state
constexpr int WAH_PEDAL_PIN          = 12;
constexpr int VOLUME_PEDAL_PIN       = 13;

// Pedal smoothing parameters
constexpr int NUM_READINGS           = 6;
const int pedalBounds[2][2]         = {{50, 2800}, {100, 3500}};  // {wah, volume}
int readings[2][NUM_READINGS]        = {{0}};
int readIndex                        = 0;
int readTotal[2]                     = {0};
int readAverage[2]                   = {0};

// Threshold for sending CC on pedal change
constexpr int THRESHOLD              = 2;
int lastValue[2]                     = {-1, -1};  // {wah, volume}

// Button state tracking
bool buttonStates[BUTTON_NUM]        = {false};
ezButton buttonArray[BUTTON_NUM]    = {
  ezButton(buttonPins[0]), ezButton(buttonPins[1]), ezButton(buttonPins[2]),
  ezButton(buttonPins[3]), ezButton(buttonPins[4]), ezButton(buttonPins[5]),
  ezButton(buttonPins[6]), ezButton(buttonPins[7]), ezButton(buttonPins[8])
};

// BLE MIDI connection flag
bool isConnected = false;

// Forward declarations
void calibration();
int smooth(int pos, int rawValue);

void setup() {
  // Initialize serial for debug (115200 baud)
  Serial.begin(115200);
  while (!Serial) {}

  // Lock CPU frequency for stable BLE timing
  setCpuFrequencyMhz(240);

  // Initialize LEDs
  for (int i = 0; i < LED_NUM; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(LED_CONN_PIN, OUTPUT);
  digitalWrite(LED_CONN_PIN, LOW);

  // Initialize pedal buffers with current readings
  for (int pos = 0; pos < 2; pos++) {
    for (int i = 0; i < NUM_READINGS; i++) {
      int raw = analogRead(pos == 0 ? WAH_PEDAL_PIN : VOLUME_PEDAL_PIN);
      int mapped = constrain(map(raw, pedalBounds[pos][0], pedalBounds[pos][1], 127, 0), 0, 127);
      readings[pos][i] = mapped;
      readTotal[pos]   += mapped;
    }
    readAverage[pos] = readTotal[pos] / NUM_READINGS;
    lastValue[pos]   = readAverage[pos];
  }

  // Initialize button debouncers
  for (int i = 0; i < BUTTON_NUM; i++) {
    buttonArray[i].setDebounceTime(50);
  }

  // Start BLE MIDI server
  BLEMidiServer.begin("Line6Floorboard");
  BLEMidiServer.setOnConnectCallback([]() {
    isConnected = true;
    digitalWrite(LED_CONN_PIN, HIGH);
  });
  BLEMidiServer.setOnDisconnectCallback([]() {
    isConnected = false;
    digitalWrite(LED_CONN_PIN, LOW);
  });

#ifdef ENABLE_CALIBRATION
  calibration();
#endif
}

void loop() {
  // Process button input
  for (int i = 0; i < BUTTON_NUM; i++) {
    buttonArray[i].loop();
    if (buttonArray[i].isPressed()) {
      buttonStates[i] = !buttonStates[i];
      digitalWrite(ledPins[i], buttonStates[i] ? HIGH : LOW);
      if (isConnected) {
        BLEMidiServer.controlChange(
          MIDI_CHANNEL,
          (uint8_t)(102 + i),  // ccNumbers: 102..110
          buttonStates[i] ? 127 : 0
        );
      }
    }
  }

  if (isConnected) {
    // Smooth and send pedal CCs
    int vals[2] = {
      smooth(0, analogRead(WAH_PEDAL_PIN)),
      smooth(1, analogRead(VOLUME_PEDAL_PIN))
    };
    for (int pos = 0; pos < 2; pos++) {
      if (abs(vals[pos] - lastValue[pos]) >= THRESHOLD) {
        uint8_t cc = (pos == 0 ? CC_MODULATION : CC_VOLUME);
        BLEMidiServer.controlChange(MIDI_CHANNEL, cc, vals[pos]);
        lastValue[pos] = vals[pos];
      }
    }
  }

  // Give FreeRTOS a moment
  delay(1);
}

// Calibration helper (for debug only)
void calibration() {
  Serial.println("Enter 'n' to start calibration");
  while (true) {
    if (Serial.available() && (Serial.read() == 'n')) {
      int minVal = 4095, maxVal = 0, sum = 0;
      for (int i = 0; i < 1000; i++) {
        int v = analogRead(VOLUME_PEDAL_PIN);
        sum += v;
        minVal = min(minVal, v);
        maxVal = max(maxVal, v);
        delay(10);
      }
      float avg = sum / 1000.0;
      Serial.printf("AVG=%.1f, MIN=%d, MAX=%d\n", avg, minVal, maxVal);
      break;
    }
    delay(100);
  }
}

// Moving-average smoothing for pedals
int smooth(int pos, int rawValue) {
  int mapped = constrain(
    map(rawValue,
        pedalBounds[pos][0], pedalBounds[pos][1],
        127, 0),
    0, 127
  );
  readTotal[pos] = readTotal[pos] - readings[pos][readIndex] + mapped;
  readings[pos][readIndex] = mapped;
  readAverage[pos] = readTotal[pos] / NUM_READINGS;
  if (pos == 1) {
    readIndex = (readIndex + 1) % NUM_READINGS;
  }
  return readAverage[pos];
}