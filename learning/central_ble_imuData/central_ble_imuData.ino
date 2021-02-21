#include <bluefruit.h>

BLEClientService        dataC(0x6969);
BLEClientCharacteristic dataC(0xAAAA);

struct sendData {
  unsigned long tickTime = 0;
  float accX = 0;
  float accY = 0;
  float accZ = 0;
  float gyroX = 0;
  float gyroY = 0;
  float gyroZ = 0;
  float magX = 0;
  float magY = 0;
  float magZ = 0;
}dataIn;

void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 Central Custom IMU Example");
  Serial.println("--------------------------------------\n");

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);

  Bluefruit.setName("adafruit_nRF_Central");

  // Initialize IMU client
  imuS.begin();

  // set up callback for receiving measurement
  dataC.setNotifyCallback(IMU_notify_callback);
  dataC.begin();

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

void loop()
{
  // do nothing
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
  if ( !dataC.discover() )
  {
    // Measurement chr is mandatory, if it is not found (valid), then disconnect
    Serial.println("not found !!!");  
    Serial.println("Measurement characteristic is mandatory but not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  Serial.println("Found it");

  // Measurement is found, continue to look for option Body Sensor Location
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
  // Body Sensor Location is optional, print out the location in text if present
  Serial.print("Discovering Body Sensor Location characteristic ... ");
  if ( bslc.discover() )
  {
    Serial.println("Found it");
    
    // Body sensor location value is 8 bit
    const char* body_str[] = { "Other", "Chest", "Wrist", "Finger", "Hand", "Ear Lobe", "Foot" };

    // Read 8-bit BSLC value from peripheral
    uint8_t loc_value = bslc.read8();
    
    Serial.print("Body Location Sensor: ");
    Serial.println(body_str[loc_value]);
  }else
  {
    Serial.println("Found NONE");
  }

  // Reaching here means we are ready to go, let's enable notification on measurement chr
  if ( dataC.enableNotify() )
  {
    Serial.println("Ready to receive IMU Measurement value");
  }else
  {
    Serial.println("Couldn't enable notify for IMU Measurement. Increase DEBUG LEVEL for troubleshooting");
  }
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
 *              in this example it should be dataC
 * @param data  Pointer to received data
 * @param len   Length of received data
 */
void IMU_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  memcpy(&dataIn, data, sizeof(dataIn));

  Serial.print(","); Serial.print(dataIn.tickTime);
  
  Serial.print(","); Serial.print(dataIn.accX);
  Serial.print(","); Serial.print(dataIn.accY);
  Serial.print(","); Serial.print(dataIn.accZ);

  Serial.print(","); Serial.print(dataIn.gyroX);
  Serial.print(","); Serial.print(dataIn.gyroY);
  Serial.print(","); Serial.print(dataIn.gyroZ);

  Serial.print(","); Serial.print(dataIn.magX);
  Serial.print(","); Serial.print(dataIn.magY);
  Serial.print(","); Serial.println(dataIn.magZ);
}
