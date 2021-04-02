# bike_computer

## Repo Structure
```
bike_computer
│   LICENSE
│   README.md
│   
├───CAD
│   ├───SolidWorks
|   |       <all solidworks parts>
|   |       <solidworks assembly>
|   |
│   ├───STEP
│   │       case_batteryHolder_prototype.STEP
│   │       case_bottom_prototype.STEP
│   │       case_top_testPrototype.STEP
│   │       hub_case_testPrototype.STEP
│   │
│   └───STL
│           case_batteryHolder_prototype.STL
│           case_bottom_Prototype.STL
│           case_hubMount_prototype.STL
│           case_top_testPrototype.STL
│
├───learning
│   ├───central_ble_imuData
│   │       central_ble_imuData.ino
│   │
│   ├───central_HRM_learning
│   │       central_HRM_learning.ino
│   │       nRF_Connect_Results.md
│   │
│   ├───imu_9dof_printData
│   │       imu_9dof_printData.ino
│   │
│   ├───Matlab Analysis
│   │       dataAnalysis.mlx
│   │       TestDataLog.xlsx
│   │
│   ├───OLED_featherwing_test
│   │       OLED_featherwing_test.ino
│   │
│   └───peripheral_ble_imuData
│           peripheral_ble_imuData.ino
│
└───SRC
    ├───central_HUD
    │       central_HUD.ino
    │
    └───peripheral_hubSensor
            peripheral_hubSensor.ino
```

### Description

A wireless bicycle Heads-up-display (HUD) that displays rotations-per-minute (RPM) and heart rate (HR). This project consists of 2 main parts, the HUD and a hub mouted speed sensor. The HUD connectects to an external heart rate sensor for HR data.

This project is designed to transform a 'dumb' indoor bicyle trainer into a more effective device by informing the user of their current RPM and HR.

#### Hardware:

##### Final product
- Adafruit Bluefruit Sense Feather
- Adafruit nRF52840 Feather
- Polar H10 Heart Rate Sensor
- Slide Switch (2)
- 400 mA Lipo (2)
- 26 AWG silicone hook-up wire
- stacking female header
- Custon 3-D printed Enclosures

##### Testing and Validation
- Hall effect sensor
- Magnets