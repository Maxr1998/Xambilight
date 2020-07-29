#include <FastLED.h>
#include <colorutils.h>

#define SERIAL_RX_BUFFER_SIZE 256

#define MODE_OFF -1
#define MODE_CLEAR 0
#define MODE_AMBILIGHT 1
#define MODE_CYCLE 2
#define MODE_FADE 3
#define MODE_COLOR 4

#define LED_PIN 5
#define CONFIG_PIN 2
#define NUM_LEDS 30
#define BUFFER_SIZE 100

#define AMBILIGHT_FADE_TIME 64

int mode = MODE_OFF;

byte message_buffer[BUFFER_SIZE];
CRGB leds[NUM_LEDS];
CRGB next_leds[NUM_LEDS];

unsigned long last_recv_ms;
int rainbow_iter = 0;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(1); // Timeout after 1 ms
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear(true);

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
  int recv_cnt;
  if ((recv_cnt = Serial.readBytesUntil('\n', message_buffer, BUFFER_SIZE)) > 0)
  {
    // Check for CMD flag
    if (message_buffer[0] == 'M' /* 0x4D */ && recv_cnt == 2)
    {
      // New mode, run setup
      switch (mode = message_buffer[1])
      {
      case MODE_CLEAR:
        FastLED.clear(true);
        mode = MODE_OFF;
        break;
      case MODE_AMBILIGHT:
        FastLED.setBrightness(200);
        break;
      case MODE_FADE:
        FastLED.setBrightness(255);
        fill_rainbow(leds, NUM_LEDS, 0, 8);
      }

      // Reply with current status
      snprintf((char *)message_buffer, BUFFER_SIZE, "{\"mode\": %d, \"led_cnt\": %d}", mode, NUM_LEDS);
      Serial.println((char *)message_buffer);

      return;
    }
    // Handle single colors
    if (mode == MODE_COLOR && message_buffer[0] == 'c' /* 0x63 */ && recv_cnt == 5)
    {
      FastLED.setBrightness((int)message_buffer[1]);
      fill_solid(leds, NUM_LEDS, CRGB((int)message_buffer[2], (int)message_buffer[3], (int)message_buffer[4]));
    }
    else
    {
      // Copy data to next LEDs for transitioning later
      for (int i = 0; i < NUM_LEDS; i++)
      {
        byte *current_buffer = message_buffer + i * 3;
        next_leds[i] = CRGB(current_buffer[0], current_buffer[1], current_buffer[2]);
      }
      last_recv_ms = millis();
    }
  }

  switch (mode)
  {
  case MODE_OFF:
    return; // noop
  case MODE_AMBILIGHT:
  {
    nblend(leds, next_leds, NUM_LEDS, 12);
    blur1d(leds, NUM_LEDS, 10);
    FastLED.delay(4);
  }
  break;
  case MODE_FADE:
    CRGB last_led = leds[NUM_LEDS - 1];
    memmove(leds + 1, leds, (NUM_LEDS - 1) * sizeof(CRGB));
    leds[0] = last_led;
    FastLED.delay(100);
    return;
  }
  FastLED.show();
}