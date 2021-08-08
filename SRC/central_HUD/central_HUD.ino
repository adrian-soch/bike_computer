/*********************************************************************
      Central                 <--->           Peripheral_1
(nRF52 BLE Feather)                     (nRF52 BLE Feather Sense)

      Central                 <--->           Peripheral_2
(nRF52 BLE Feather)                           (Polar H10)

 - This sketch to a BLE feather board
 - Peripheral sketch to a BLE feather Sense
 - Open a serial connection to view the 9 dof IMU data from the peripheral


*********************************************************************/

#include <bluefruit.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define WIDTH 128
#define HEIGHT 64

// BLE 
/* HRM Service Definitions
 * Heart Rate Monitor Service:  0x180D
 * Heart Rate Measurement Char: 0x2A37 (Mandatory)
 * Body Sensor Location Char:   0x2A38 (Optional)
 * Model Number:                0x2A25
 */

BLEClientService        hrms(UUID16_SVC_HEART_RATE);
BLEClientCharacteristic hrmc(UUID16_CHR_HEART_RATE_MEASUREMENT);
BLEClientCharacteristic bslc(UUID16_CHR_BODY_SENSOR_LOCATION);

BLEClientService        imuS(0x6969);
BLEClientCharacteristic gyroC(0xBBBB);

// OLED
Adafruit_SH110X display = Adafruit_SH110X(HEIGHT, WIDTH, &Wire);

struct data {
  float gyroX = 0;
  float gyroY = 0;
  float gyroZ = 0;
  float batteryLevel = 0;
}dataIn;

int16_t rpm_filtered = 0;
const float radPerSec2RPM = 30/3.141592654; // (2*pi/60)^-1
unsigned long previous = 0;

void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  // OLED
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");
  display.display();
  delay(3000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  display.setRotation(1);

  // text display tests
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setCursor(0,0);

  // BLE
  Serial.println("Starting BLE...");
  Serial.println("--------------------------------------\n");

  // Initialize Bluefruit with maximum connections as Peripheral = X, Central = Y
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 2);

  Bluefruit.setName("HUD_Central");

  // Initialize IMU client
  imuS.begin();

  // set up callback for receiving measurement
  gyroC.setNotifyCallback(IMU_notify_callback);
  gyroC.begin();

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250);

  // Callbacks for Central
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Don't use active scan
   * - Filter only accept IMU service
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.filterUuid(imuS.uuid);
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);                   // // 0 = Don't stop scanning after n seconds
}

/**
 * @brief Moving average filter
 * @param rpm A single rpm measurement
 * @param rpm_filtered Allocted variable to store result
 */
void movingAverage(const int16_t rpm, int16_t *rpm_filtered)
{
  const uint8_t num_windows = 7;
  static int16_t rpmArray[num_windows] = {0};
  static uint8_t ii = 0;
  int32_t sum = 0;

  rpmArray[ii++ % num_windows] = rpm;

  for(uint8_t jj = 0; jj < num_windows; jj++){
    sum += rpmArray[jj];
  }
  *rpm_filtered = sum/num_windows;
}

void loop()
{  
  if(previous - millis() >= 125) {
    previous = millis();
    
    int16_t rpm = rpm_filtered;
    if( abs(rpm) <= 4 ){
      rpm = 0;
    }
    char buf[100] = {};
    sprintf(buf, "RPM: %hd      \nVp: %.1f", rpm, dataIn.batteryLevel);
    display.setCursor(0,0);
    display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    display.print(buf);
    yield();
    display.display();
  }
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  // Since we configure the scanner with filterUuid()
  // Scan callback only invoked for device with IMU service advertised
  // Connect to device with IMU service in advertising
  Bluefruit.Central.connect(report);
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");
  Serial.print("Discovering IMU Service ... ");

  // If IMU is not found, disconnect and return
  if ( !imuS.discover(conn_handle) ){
    Serial.println("Found NONE");

    // disconnect since we couldn't find IMU service
    Bluefruit.disconnect(conn_handle);

    return;
  }

  // Once IMU service is found, we continue to discover its characteristic
  Serial.println("Found it");
  
  Serial.print("Discovering Measurement characteristic ... ");
  if ( !gyroC.discover() ){
    // Measurement chr is mandatory, if it is not found (valid), then disconnect
    Serial.println("not found !!!");  
    Serial.println("Measurement characteristic is mandatory but not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }

  if ( gyroC.enableNotify() ){
    Serial.println("Ready to receive gyro Measurement value");
  }
  else{
    Serial.println("Couldn't enable notify for gyro Measurement. Increase DEBUG LEVEL for troubleshooting");
  }

  display.clearDisplay();
  display.setCursor(0,0);
  yield();
  display.display();
  delay(25);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{

  (void) conn_handle;
  (void) reason;
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
  
  dataIn.batteryLevel = 0;
  rpm_filtered = 0; 
}


/**
 * Hooked callback that triggered when a measurement value is sent from peripheral
 * @param chr   Pointer client characteristic that even occurred,
 *              in this example it should be gyroC
 * @param data  Pointer to received data
 * @param len   Length of received data
 */
void IMU_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  memcpy(&dataIn, data, sizeof(dataIn));
  movingAverage((int16_t)dataIn.gyroY*radPerSec2RPM, &rpm_filtered);
  Serial.print("Gyro :");
  char buf[150];
  sprintf(buf, "%.3f,%hd,%.2f", dataIn.gyroY, rpm_filtered, dataIn.batteryLevel);
  Serial.println(buf);
}
