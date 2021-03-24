#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define WIDTH 128
#define HEIGHT 64

Adafruit_SH110X display = Adafruit_SH110X(HEIGHT, WIDTH, &Wire);

void setup() {
  Serial.begin(115200);

  Serial.println("128x64 OLED FeatherWing test");
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(300);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  display.setRotation(1);

  // text display tests
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setCursor(0,0);
}

void loop() {
  static int accel = 0;
  static int gyro = 0;
  display.setCursor(0,0);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  display.print("Accel: "); display.println(accel++);
  display.print("Gyro: "); display.println(gyro++);
  
  delay(50);
  yield();
  display.display();
}
