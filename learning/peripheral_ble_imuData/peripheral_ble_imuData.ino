/*********************************************************************
 Central                 <--->    Peripheral
 (nRF52 BLE Feather)              (nRF52 BLE Feather Sense)

 - This sketch to a BLE feather Sense board
 - Central sketch to a BLE feather
 - Move the Feather sense to view 9 dof IMU data into the central's serial output


*********************************************************************/

#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS33.h>
#include <bluefruit.h>

#define PACKET_SIZE 16 // 4*4

Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LIS3MDL lis3mdl;

BLEService        imuS = BLEService(0x6969);  //imu service
BLECharacteristic accelC = BLECharacteristic(0xAAAA);
BLECharacteristic gyroC = BLECharacteristic(0xBBBB);
BLECharacteristic magC = BLECharacteristic(0xCCCC);

//BLEDis bleDIS;    // DIS (Device Information Service) helper class instance
BLEBas bleBATT;    // BAS (Battery Service) helper class instance

struct accelData {
  unsigned long tickTime = 0;
  float accX = 0;
  float accY = 0;
  float accZ = 0;
}accelDataOut;

struct gyroData {
  unsigned long tickTime = 0;
  float gyroX = 0;
  float gyroY = 0;
  float gyroZ = 0;
}gyroDataOut;

struct magData {
  unsigned long tickTime = 0;
  float magX = 0;
  float magY = 0;
  float magZ = 0;
}magDataOut;

void setup(void) {
  Bluefruit.begin();
  Bluefruit.setName("adafruit_nRF_IMU");

  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

//  bledis.setManufacturer("Adafruit Industries");
//  bledis.setModel("Bluefruit Feather52");
//  bledis.begin();

  setupBLE();

  startAdvertising();
  
  if (!lsm6ds33.begin_I2C()) {
    Serial.println("Failed to find LSM6DS33 chip");
    while (1) {delay(10);}
  }

  if (! lis3mdl.begin_I2C()){
    Serial.println("Failed to find LIS3MDL chip");
    while (1) {delay(10);}
  }
 
  lsm6ds33.configInt1(false, false, true); // accelerometer DRDY on INT1
  lsm6ds33.configInt2(false, true, false); // gyro DRDY on INT2

  lis3mdl.setPerformanceMode(LIS3MDL_MEDIUMMODE);
  lis3mdl.setOperationMode(LIS3MDL_CONTINUOUSMODE);
  lis3mdl.setDataRate(LIS3MDL_DATARATE_155_HZ);
  lis3mdl.setRange(LIS3MDL_RANGE_4_GAUSS);
  
  lis3mdl.setIntThreshold(500);
  lis3mdl.configInterrupt(false, false, true, // enable z axis
                          true, // polarity
                          false, // don't latch
                          true); // enabled!
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
  accelC.setProperties(CHR_PROPS_NOTIFY);
  accelC.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  accelC.setFixedLen(PACKET_SIZE); 
  //accelC.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  accelC.begin();
  uint8_t acceldata[PACKET_SIZE] = {0}; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  accelC.write(acceldata, PACKET_SIZE);

  gyroC.setProperties(CHR_PROPS_NOTIFY);
  gyroC.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  gyroC.setFixedLen(PACKET_SIZE); 
  //gyroC.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  gyroC.begin();
  uint8_t gyrodata[PACKET_SIZE] = {0}; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  gyroC.write(gyrodata, PACKET_SIZE);

  magC.setProperties(CHR_PROPS_NOTIFY);
  magC.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  magC.setFixedLen(PACKET_SIZE); 
  //magC.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  magC.begin();
  uint8_t magdata[PACKET_SIZE] = {0}; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  magC.write(magdata, PACKET_SIZE);
}

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

//  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}


void loop() {
  digitalToggle(LED_RED);

  if ( Bluefruit.connected() ) {
    /* Get a new normalized sensor event */
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    lsm6ds33.getEvent(&accel, &gyro, &temp);
    
    sensors_event_t magno; 
    lis3mdl.getEvent(&magno);

    accelDataOut.tickTime = millis();
    accelDataOut.accX = accel.acceleration.x;
    accelDataOut.accY = accel.acceleration.y;
    accelDataOut.accY = accel.acceleration.z;
    accelC.notify((const uint8_t*)&accelDataOut, sizeof(accelDataOut));
    
    gyroDataOut.tickTime = millis();
    gyroDataOut.gyroX = gyro.gyro.x;
    gyroDataOut.gyroY = gyro.gyro.y;
    gyroDataOut.gyroZ = gyro.gyro.z;
    gyroC.notify((const uint8_t*)&gyroDataOut, sizeof(gyroDataOut));
    
    magDataOut.tickTime = millis();
    magDataOut.magX = magno.magnetic.x;
    magDataOut.magY = magno.magnetic.y; 
    magDataOut.magZ = magno.magnetic.z; 
    magC.notify((const uint8_t*)&magDataOut, sizeof(magDataOut));
  }
    
  delay(50);
}
