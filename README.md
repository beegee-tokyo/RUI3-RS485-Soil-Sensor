# RUI3-RS485-Soil-Sensor
| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: |    

Example for a RS485 soil sensor using RAKwireless RUI3 on a RAK3172

# Components

----

## Soil sensor
- VMSEE SN-3002-TR-ECTHNPKKPH-N01, translated datasheet is in [assets](./assets/SoilSensor-7-values-datasheet_en.docx)

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
   
