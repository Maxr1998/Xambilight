#include <FastLED.h>
#include <colorutils.h>

#define MODE_EXIT -1
#define MODE_IDLE 0
#define MODE_AMBILIGHT 1
#define MODE_CYCLE 2
#define MODE_FADE 3
#define MODE_COLOR 4
#define MODE_OFF 5

#define LED_PIN 3
#define NUM_LEDS 30

int mode = MODE_IDLE;

char message_buffer[200];
int bytes;
CRGB leds[NUM_LEDS];

int iterator = 0;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20); // Timeout after 100 ms
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  // Boot flag
  pinMode(2, INPUT_PULLUP);
  if (digitalRead(2) == LOW) {
    mode = MODE_FADE;
  }
}

void loop() {
  // Check for new commands
  bytes = 0;
  memset(message_buffer, 0, sizeof(message_buffer));
  if ((bytes = Serial.readBytesUntil('\n', (char*) message_buffer, 200)) > 0) {
    // Check for CMD flag
    if (message_buffer[0] == 'M' /* 0x4D */ && bytes == 2) {
      // New mode available
      mode = message_buffer[1];
      Serial.print("New mode " + mode);
      return;
    }
    // Handle single colors
    if (mode == MODE_COLOR) {
      if (message_buffer[0] == 'c' /* 0x63 */ && bytes == 5) {
        FastLED.setBrightness((int) message_buffer[1]);
        fill_solid(leds, NUM_LEDS, CRGB((int) message_buffer[2], (int) message_buffer[3], (int) message_buffer[4]));
      }
    } else {
      FastLED.setBrightness(255);
      // Copy to LEDs
      memcpy((char*) leds, message_buffer, sizeof leds);
    }
  }

  switch (mode) {
    case MODE_AMBILIGHT:
    case MODE_CYCLE:
    case MODE_COLOR:
      FastLED.show();
      break;
    case MODE_FADE:
      fill_rainbow(leds, NUM_LEDS, iterator, 8);
      iterator += 2;
      if (iterator > 255) {
        iterator = 0;
      }
      FastLED.show();
      break;
    case MODE_IDLE:
      FastLED.clear(true);
      mode = MODE_EXIT;
      break;
    default:
      break;
  }
}
