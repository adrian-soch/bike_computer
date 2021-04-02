/*********************************************************************
        Peripheral_1           <--->         Central         
(nRF52 BLE Feather Sense)               (nRF52 BLE Feather)

 - Upload this sketch to a BLE feather Sense board
 - Upload Central sketch to a BLE feather attached to OLED
*********************************************************************/

#include <Adafruit_LSM6DS33.h>
#include <bluefruit.h>

#define PACKET_SIZE 16 // float (4 bytes)*4 = 16
#define VBATPIN A6

Adafruit_LSM6DS33 lsm6ds33;

BLEService        imuS = BLEService(0x6969);          //imu service
BLECharacteristic gyroC = BLECharacteristic(0xBBBB);  //gyro characteristic 

struct data {
  float gyroX = 0;
  float gyroY = 0;
  float gyroZ = 0;
  float batteryLevel = 0;
}dataOut;

void setup(void)
{
  /* Sensor setup */
    if (!lsm6ds33.begin_I2C()) {
    Serial.println("Failed to find LSM6DS33 chip");
    while (1) {
      digitalToggle(LED_RED);
      delay(10);
      }
  }
  lsm6ds33.configInt1(false, false, true); // accelerometer DRDY on INT1
  lsm6ds33.configInt2(false, true, false); // gyro DRDY on INT2
  lsm6ds33.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  lsm6ds33.highPassFilter(1, LSM6DS_HPF_ODR_DIV_50);

  /* BLE setup */
  Bluefruit.begin();
  Bluefruit.setName("adafruit_nRF_IMU");

  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  setupBLE();
  startAdvertising();
}

void startAdvertising(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(imuS);

  // Include Name
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);   
}

void setupBLE(void)
{
  imuS.begin();
  gyroC.setProperties(CHR_PROPS_NOTIFY);
  gyroC.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  gyroC.setFixedLen(PACKET_SIZE); 
  //gyroC.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  gyroC.begin();
  uint8_t gyrodata[PACKET_SIZE] = {0}; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  gyroC.write(gyrodata, PACKET_SIZE);
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = {0};
  connection->getPeerName(central_name, sizeof(central_name));
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
}


void loop()
{
  digitalToggle(LED_RED);
  if ( Bluefruit.connected() ) {
    /* Get a new normalized sensor event */
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    lsm6ds33.getEvent(&accel, &gyro, &temp);

    // Adjeust gyra vals based on calculated offset
    // Gyro offsets calibrated from Adafruit sensorLab example
    dataOut.gyroX = gyro.gyro.x - 0.0383;
    dataOut.gyroY = gyro.gyro.y + 0.1830;
    dataOut.gyroZ = gyro.gyro.z + 0.0529;

    // Get battery voltage
    dataOut.batteryLevel = analogRead(VBATPIN);
    dataOut.batteryLevel *= 2;    // we divided by 2, so multiply back
    dataOut.batteryLevel *= 3.3;  // Multiply by 3.3V, our reference voltage
    dataOut.batteryLevel /= 1024; // convert to voltage

    // Send packet
    gyroC.notify((const uint8_t*)&dataOut, sizeof(dataOut));
  }
  delay(50);
}
