#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS33.h>
#include <bluefruit.h>

#define PACKET_SIZE 40 //10*4

Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LIS3MDL lis3mdl;

BLEService        imuS = BLEService(0x6969);  //imu service
BLECharacteristic dataC = BLECharacteristic(0xAAAA);

//BLEDis bleDIS;    // DIS (Device Information Service) helper class instance
BLEBas bleBATT;    // BAS (Battery Service) helper class instance

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
} dataToSend;

void setup(void) {
  Bluefruit.begin();
  Bluefruit.setName("nRF IMU Test");

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
  dataC.setProperties(CHR_PROPS_NOTIFY);
  dataC.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  dataC.setFixedLen(PACKET_SIZE); 
  //dataC.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  dataC.begin();
  uint8_t imudata[40] = {0}; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  dataC.write(imudata, PACKET_SIZE);
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

    dataToSend.tickTime = millis();
    dataToSend.accX = accel.acceleration.x;
    dataToSend.accY = accel.acceleration.y;
    dataToSend.accY = accel.acceleration.z;
    dataToSend.gyroX = gyro.gyro.x;
    dataToSend.gyroY = gyro.gyro.y;
    dataToSend.gyroZ = gyro.gyro.z;
    dataToSend.magX = magno.magnetic.x;
    dataToSend.magY = magno.magnetic.y; 
    dataToSend.magZ = magno.magnetic.z; 
  
    dataC.notify((const uint8_t*)&dataToSend, sizeof(dataToSend));
  }
    
  delay(100);
}
