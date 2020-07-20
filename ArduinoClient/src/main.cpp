#include <FastLED.h>
#include <colorutils.h>

#define MODE_OFF -1
#define MODE_CLEAR 0
#define MODE_AMBILIGHT 1
#define MODE_CYCLE 2
#define MODE_FADE 3
#define MODE_COLOR 4

#define LED_PIN 5
#define CONFIG_PIN 2
#define NUM_LEDS 30
#define BUFFER_SIZE 200

int mode = MODE_CLEAR; // Initialize LEDs on startup

int recv_cnt;
char message_buffer[BUFFER_SIZE];
CRGB leds[NUM_LEDS];

int iterator = 0;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(20); // Timeout after 100 ms
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  // Boot flag
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  if (digitalRead(CONFIG_PIN) == LOW)
  {
    mode = MODE_FADE;
  }
}

void loop()
{
  // Check for new commands
  memset(message_buffer, 0, BUFFER_SIZE);
  if ((recv_cnt = Serial.readBytesUntil('\n', (char *)message_buffer, BUFFER_SIZE)) > 0)
  {
    // Check for CMD flag
    if (message_buffer[0] == 'M' /* 0x4D */ && recv_cnt == 2)
    {
      // New mode available
      mode = message_buffer[1];
      snprintf(message_buffer, BUFFER_SIZE, "{\"mode\": %d, \"led_cnt\": %d}", mode, NUM_LEDS);
      Serial.println(message_buffer);
      return;
    }
    // Handle single colors
    if (mode == MODE_COLOR)
    {
      if (message_buffer[0] == 'c' /* 0x63 */ && recv_cnt == 5)
      {
        FastLED.setBrightness((int)message_buffer[1]);
        fill_solid(leds, NUM_LEDS, CRGB((int)message_buffer[2], (int)message_buffer[3], (int)message_buffer[4]));
      }
    }
    else
    {
      FastLED.setBrightness(255);
      // Copy data to LEDs
      memcpy((char *)leds, message_buffer, sizeof leds);
    }
  }

  switch (mode)
  {
  case MODE_OFF:
    // noop
    break;
  case MODE_CLEAR:
    FastLED.clear(true);
    mode = MODE_OFF;
    break;
  case MODE_FADE:
    fill_rainbow(leds, NUM_LEDS, iterator, 8);
    iterator = (iterator + 2) % 256;
    FastLED.show();
    break;
  default:
    FastLED.show();
    break;
  }
}