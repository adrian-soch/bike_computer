/*********************************************************************
      Central                 <--->           Peripheral_1
(nRF52 BLE Feather)                     (nRF52 BLE Feather Sense)

      Central                 <--->           Peripheral_2
(nRF52 BLE Feather)                           (Polar H10)

 - Upload this sketch to a BLE feather board with OLED attached
 - Upload the Peripheral sketch to a BLE feather Sense
 - Activate HR sensor (Polar H10)
*********************************************************************/

#include <bluefruit.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define WIDTH 128
#define HEIGHT 64

// BLE 
BLEClientService        imuS(0x6969);
BLEClientCharacteristic accelC(0xAAAA);
BLEClientCharacteristic gyroC(0xBBBB);
BLEClientCharacteristic magC(0xCCCC);

// OLED
Adafruit_SH110X display = Adafruit_SH110X(HEIGHT, WIDTH, &Wire);

struct imuData {
  unsigned long tickTime = 0;
  float x = 0;
  float y = 0;
  float z = 0;
}dataIn;

void setup()
{
  Serial.begin(115200);

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

  Bluefruit.setName("adafruit_nRF_Central");

  // Initialize IMU client
  imuS.begin();

  // set up callback for receiving measurement
  accelC.setNotifyCallback(IMU_notify_callback);
  accelC.begin();

  // set up callback for receiving measurement
  gyroC.setNotifyCallback(IMU_notify_callback);
  gyroC.begin();

  // set up callback for receiving measurement
  magC.setNotifyCallback(IMU_notify_callback);
  magC.begin();

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

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.print("Connecting...");
  display.display();
}

void loop()
{  
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
  if ( !imuS.discover(conn_handle) )
  {
    Serial.println("Found NONE");

    // disconnect since we couldn't find IMU service
    Bluefruit.disconnect(conn_handle);

    return;
  }

  // Once IMU service is found, we continue to discover its characteristic
  Serial.println("Found it");
  
  Serial.print("Discovering Measurement characteristic ... ");
  if ( !accelC.discover() & !gyroC.discover() & !magC.discover())
  {
    // Measurement chr is mandatory, if it is not found (valid), then disconnect
    Serial.println("not found !!!");  
    Serial.println("Measurement characteristic is mandatory but not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }

  // Reaching here means we are ready to go, let's enable notification on measurement chr
  if ( accelC.enableNotify() )
  {
    Serial.println("Ready to receive accel Measurement value");
  }else
  {
    Serial.println("Couldn't enable notify for accel Measurement. Increase DEBUG LEVEL for troubleshooting");
  }

  if ( gyroC.enableNotify() )
  {
    Serial.println("Ready to receive gyro Measurement value");
  }else
  {
    Serial.println("Couldn't enable notify for gyro Measurement. Increase DEBUG LEVEL for troubleshooting");
  }

  if ( magC.enableNotify() )
  {
    Serial.println("Ready to receive mag Measurement value");
  }else
  {
    Serial.println("Couldn't enable notify for mag Measurement. Increase DEBUG LEVEL for troubleshooting");
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connected!\n");
  display.display();
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
}


/**
 * Hooked callback that triggered when a measurement value is sent from peripheral
 * @param chr   Pointer client characteristic that even occurred,
 *              in this example it should be accelC, gyroC or magC
 * @param data  Pointer to received data
 * @param len   Length of received data
 */
void IMU_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  memcpy(&dataIn, data, sizeof(dataIn));
  Serial.print(chr->valueHandle());
  
//  if(chr->valueHandle() == 16){
//    Serial.print("Accel :");
//  }
//  else if(chr->valueHandle() == 19){
//    Serial.print("Gyro :");
//  }
//  else if(chr->valueHandle() == 22){
//    Serial.print("Mag :");
//  }
//  else{
//    // Not an expected characteristic, ignore it+
//    Serial.print("Skipping, check connHandle value");
//    return;
//  }
  
  Serial.print(","); Serial.print(dataIn.tickTime);
  Serial.print(","); Serial.print(dataIn.x);
  Serial.print(","); Serial.print(dataIn.y);
  Serial.print(","); Serial.println(dataIn.z);

}
