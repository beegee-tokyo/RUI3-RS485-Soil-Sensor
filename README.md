# RUI3-RS485-Soil-Sensor
| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: |    

Example for a RS485 soil sensor using RAKwireless RUI3 on a RAK3172.

Tested with the "No-Name" VEM SEE SN-3002-TR-ECTHNPKKPH-N01 sensor.
Code prepared for the GEMHO 7in1 Soil Sensor with RS485, but not tested.

# Components

----

## Soil sensor

### VEM SEE SN-3002-TR-ECTHNPKKPH-N01, translated datasheet is in [assets](./assets/SoilSensor-7-values-datasheet_en.docx)

#### ⚠️ IMPORTANT ⚠️ Requires to set #define VEMSEE

Sensor works by default with 4800 Baud

VEM SEE SN-3002-TR-ECTHNPKKPH-N01 Soil Sensor register setup
| Address | Multiplier | Register content                              |
| ------- | ---------- | --------------------------------------------- |
| 0x0000  | /10        | Moisture                                      | 
| 0x0001  | /10        | Temperature                                   | 
| 0x0002  | *1         | Conductivity                                  | 
| 0x0003  | /10        | pH                                            | 
| 0x0004  | *1         | Nitrogen content (temporary)                  | 
| 0x0005  | *1         | Phosphorus content (temporary)                | 
| 0x0006  | *1         | Potassium content (temporary)                 | 
| 0x0007  | *1         | Salinity                                      | 
| 0x0008  | *1         | TDS (for reference ?????)                     | 
| 0x0022  | *1         | Temperature coefficient of conductivity       | 
| 0x0023  | *1         | TDS coefficient                               | 
| 0x0050  | *1         | Temperature calibration value                 | 
| 0x0051  | *1         | Mositure content calibration value            | 
| 0x0052  | *1         | Conductivity calibration value                | 
| 0x0053  | *1         | pH calibration value                          | 
| 0x04e8  | *1         | Nitrogen content coefficient MSB (temporary)  | 
| 0x04e9  | *1         | Nitrogen content coefficient LSB (temporary)  | 
| 0x04ea  | *1         | Nitrogen deviation (temporary)                | 
| 0x04f2  | *1         | Phosphorus coefficient MSB (temporary)        | 
| 0x04f3  | *1         | Phosphorus coefficient LSB (temporary)        | 
| 0x04f4  | *1         | Phosphorus deviation (temporary)              | 
| 0x04fc  | *1         | Potassium content coefficient MSB (temporary) | 
| 0x04fd  | *1         | Potassium content coefficient LSB (temporary) | 
| 0x04fe  | *1         | Potassium deviation (temporary)               | 
| 0x07d0  | *1         | Device address                                | 
| 0x07d1  | *1         | Baud Rate                                     | 

----

### GEMHO 7in1 Soil Sensor with RS485

#### ⚠️ IMPORTANT ⚠️ Requires to set #define GEMHO

Sensor works by default with 9600 Baud

GEMHO 7in1 Soil Sensor with RS485 register setup
| Address | Multiplier | Register content                              |
| ------- | ---------- | --------------------------------------------- |
| 0x0006  | /100       | Temperature                                   | 
| 0x0007  | /100       | Moisture                                      | 
| 0x0008  | *1         | Conductivity                                  | 
| 0x0009  | /100       | pH                                            | 
| 0x000F  | *1         | Device address                                | 
| 0x001E  | *1         | Nitrogen content (temporary)                  | 
| 0x001F  | *1         | Phosphorus content (temporary)                | 
| 0x0020  | *1         | Potassium content (temporary)                 | 

----

## WisBlock modules & enclosure
- [RAK3172 Evaluation Board](https://docs.rakwireless.com/product-categories/wisduo/rak3172-evaluation-board/overview/) with
   - [RAK19007](https://docs.rakwireless.com/product-categories/wisblock/rak19007/overview) WisBlock Base Board
   - [RAK3372](https://docs.rakwireless.com/product-categories/wisblock/rak3372/overview) WisBlock Core module with STM32WLE5
- [RAK5802-M](https://docs.rakwireless.com/product-categories/wisblock/rak5802/overview) WisBlock RS485 module (modified variant)
- [RAK19002](https://docs.rakwireless.com/product-categories/wisblock/rak19002/overview) WisBlock 12V booster for supply of soil sensor
- [Unify Enclosure 150x100x45 with Solar Panel](https://docs.rakwireless.com/product-categories/wisblock/rakbox-uo150x100x45-solar/overview/)
- 3200mAh battery

----

# Assembly

Assembly is done with the "standard" mounting plate of the Unify Enclosure.    
Sensor connection is done with the 5-pin IP65 connector of the Unify Enclosure with Solar Panel
Antenna used is [Blade Antenna](https://docs.rakwireless.com/Product-Categories/Accessories/RAKARJ16/Overview/) with 2.3 dBi gain.

<center><img src="./assets/assembly.jpg" width="35%" alt="Device">&nbsp&nbsp&nbsp&nbsp<img src="./assets/sensor.jpg" height="50%" width="35%" alt="Sensor"></center>

#### ⚠️ IMPORTANT ⚠️    
RAK19002 12V booster _**must**_ be installed in the Sensor Slot B    

----

# Wiring diagram

<center><img src="./assets/wiring.png" alt="Wiring Diagram"></center>

----

# Firmware

Firmware is based on [RUI3-RAK5802-Modbus-Master](https://github.com/RAKWireless/RUI3-Best-Practice/tree/main/ModBus/RUI3-RAK5802-Modbus-Master) with adjustements for the used RS485 sensor.

To achieve good sensor readings, the sensor is powered up for 10 minutes before the sensor data is read. This gives the sensor time to do the readings and calculations.

----

## Custom AT commands

Send interval of the sensor values can be set with a custom AT command. Interval time is set in _**seconds**_

_**`ATC+SENDINT?`**_ Command definition
> ATC+SENDINT,: Set/Get the interval sending time values in seconds 0 = off, max 2,147,483 seconds    
OK

_**`ATC+SENDINT=?`**_ Get current send interval in seconds
> ATC+SENDINT=3600    
OK

_**`ATC+SENDINT=3600`**_ Get current send interval to 3600 seconds == 1 hour
> ATC+SENDINT=3600    
OK

#### ⚠️ IMPORTANT ⚠️  
Send interval cannot be less than 2 times the sensor power on time. With the current settings the minimum send interval is 20 minutes

----

## Write to coils or registers

To control the coils, a downlink from the LoRaWAN server is required. The downlink packet format is     
`AA55ccddnnv1v2` as hex values       
`AA55` is a simple packet marker       
`cc` is the command, supported is only MB_FC_WRITE_MULTIPLE_COILS    
`dd` is the slave address    
`nn` is the number of coils to write     
`v1`, `v2` are the coil status. 0 ==> coil off, 1 ==> coil on, `nn` status are expected     

To write to registers, a downlink from the LoRaWAN server is required. The downlink packet format is     
`AA55ccddnnv1v2` as hex values       
`AA55` is a simple packet marker       
`cc` is the command, supported are MB_FC_WRITE_REGISTER and MB_FC_WRITE_MULTIPLE_REGISTERS    
`dd` is the slave address    
`aa` is the start address of the registers
`nn` is the number of registers to write     
if MB_FC_WRITE_REGISTER    
	`v1` and `v2` is the 16bit value to write to the register    
if MB_FC_WRITE_MULTIPLE_REGISTERS    
	`v1` and `v2` are the 16bit value to write to the register, `nn` arrays of `v1` and `v2` are expected    
   
