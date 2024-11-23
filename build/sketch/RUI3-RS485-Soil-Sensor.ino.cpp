#include <Arduino.h>
#line 1 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
/**
 * @file main.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Modbus Master reading data from environment sensors
 * @version 0.1
 * @date 2024-11-12
 *
 * @copyright Copyright (c) 2024
 *
 */

/** Soil Sensor register setup:
 * 0x0000	/10		Moisture
 * 0x0001 	/10		Temperature
 * 0x0002 	*1		Conductivity
 * 0x0003 	/10		pH
 * 0x0004 	???		Nitrogen content (temporary)
 * 0x0005 	???		Phosphorus content (temporary)
 * 0x0006 	???		Potassium content (temporary)
 * 0x0007 	???		Salinity
 * 0x0008 	???		TDS (for reference ?????)
 *
 * 0x0022 Temperature coefficient of conductivity
 * 0x0023 TDS coefficient
 *
 * 0x0050 Temperature calibration value
 * 0x0051 Mositure content calibration value
 * 0x0052 Conductivity calibration value
 * 0x0053 pH calibration value
 *
 * 0x04e8 Nitrogen content coefficient MSB (temporary)
 * 0x04e9 Nitrogen content coefficient LSB (temporary)
 * 0x04ea Nitrogen deviation (temporary)
 * 0x04f2 Phosphorus coefficient MSB (temporary)
 * 0x04f3 Phosphorus coefficient LSB (temporary)
 * 0x04f4 Phosphorus deviation (temporary)
 * 0x04fc Potassium content coefficient MSB (temporary)
 * 0x04fd Potassium content coefficient LSB (temporary)
 * 0x04fe Potassium deviation (temporary)
 *
 * 0x07d0 Device address
 * 0x07d1 Baud Rate
 */

#include "app.h"

/** Data array for modbus 9 registers */
union coils_n_regs_u coils_n_regs = {0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  port : serial port
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 */
Modbus master(0, Serial1, 0); // this is master and RS-232 or USB-FTDI

/** This is an structure which contains a query to an slave device */
modbus_t telegram;

/** This is the structure which contains a write to set/reset coils */
struct coil_s
{
	int8_t dev_addr = 1;
	int8_t coils[16];
	int8_t num_coils = 0;
};

/** Coils structure */
coil_s coil_data;

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** Payload buffer */
WisCayenne g_solution_data(255);

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
#line 91 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void joinCallback(int32_t status);
#line 111 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void receiveCallback(SERVICE_LORA_RECEIVE_T *data);
#line 176 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void sendCallback(int32_t status);
#line 187 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void recv_cb(rui_lora_p2p_recv_t data);
#line 245 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void send_cb(void);
#line 256 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void cad_cb(bool result);
#line 265 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void setup();
#line 379 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void modbus_start_sensor(void *);
#line 387 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void modbus_read_register(void *);
#line 495 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void modbus_write_coil(void *);
#line 549 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void loop(void);
#line 560 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void send_packet(void);
#line 91 "D:\\#Github\\Solutions-RUI3\\RUI3-RS485-Soil-Sensor\\RUI3-RS485-Soil-Sensor.ino"
void joinCallback(int32_t status)
{
	if (status != 0)
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
		// To be checked if this makes sense
		// api.lorawan.join();
	}
	else
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
	}
}

/**
 * @brief LoRaWAN callback after packet was received
 *
 * @param data pointer to structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");

	// Check for command fPort
	if (data->Port == 0)
	{
		MYLOG("RX-CB", "MAC command");
		return;
	}
	// Check for valid command sequence
	if ((data->Buffer[0] == 0xAA) && (data->Buffer[1] == 0x55))
	{
		// Check for command (only MB_FC_WRITE_MULTIPLE_COILS supported atm)
		if (data->Buffer[2] == MB_FC_WRITE_MULTIPLE_COILS)
		{
			// Get slave address
			coil_data.dev_addr = data->Buffer[3];
			if ((coil_data.dev_addr > 0) && (coil_data.dev_addr < 17))
			{
				// Get number of coils
				coil_data.num_coils = data->Buffer[4];

				// Check for coil number in range (1 to 16)
				if ((coil_data.num_coils > 0) && (coil_data.num_coils < 17))
				{
					// Save coil status
					for (int idx = 0; idx < coil_data.num_coils; idx++)
					{
						coil_data.coils[idx] = data->Buffer[5 + idx];
					}
					// Start a timer to handle the incoming coil write request.
					api.system.timer.start(RAK_TIMER_1, 100, NULL);
				}
				else
				{
					MYLOG("RX_CB", "Wrong num of coils");
				}
			}
			else
			{
				MYLOG("RX_CB", "invalid slave address");
			}
		}
		else
		{
			MYLOG("RX_CB", "Wrong command");
		}
	}
	else
	{
		MYLOG("RX_CB", "Wrong format");
	}
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);
	digitalWrite(LED_BLUE, LOW);
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data pointer to the data with the received data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");

	// Check for valid command sequence
	if ((data.Buffer[0] == 0xAA) && (data.Buffer[1] == 0x55))
	{
		// Check for command (only MB_FC_WRITE_MULTIPLE_COILS supported atm)
		if (data.Buffer[2] == MB_FC_WRITE_MULTIPLE_COILS)
		{
			// Get slave address
			coil_data.dev_addr = data.Buffer[3];
			if ((coil_data.dev_addr > 0) && (coil_data.dev_addr < 17))
			{
				// Get number of coils
				coil_data.num_coils = data.Buffer[4];

				// Check for coil number in range (1 to 16)
				if ((coil_data.num_coils > 0) && (coil_data.num_coils < 17))
				{
					// Save coil status
					for (int idx = 0; idx < coil_data.num_coils; idx++)
					{
						coil_data.coils[idx] = data.Buffer[5 + idx];
					}
					// Start a timer to handle the incoming coil write request.
					api.system.timer.start(RAK_TIMER_1, 100, NULL);
				}
				else
				{
					MYLOG("RX_CB", "Wrong num of coils");
				}
			}
			else
			{
				MYLOG("RX_CB", "invalid slave address");
			}
		}
		else
		{
			MYLOG("RX_CB", "Wrong command");
		}
	}
	else
	{
		MYLOG("RX_CB", "Wrong format");
	}
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
}

/**
 * @brief LoRa P2P callback for CAD result
 *
 * @param result true if activity was detected, false if no activity was detected
 */
void cad_cb(bool result)
{
	MYLOG("CAD-P2P-CB", "P2P CAD reports %s", result ? "activity" : "no activity");
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		g_confirmed_mode = api.lorawan.cfm.get();

		g_confirmed_retry = api.lorawan.rety.get();

		g_data_rate = api.lorawan.dr.get();

		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	else // Setup for LoRa P2P
	{
		api.lora.registerPRecvCallback(recv_cb);
		api.lora.registerPSendCallback(send_cb);
		api.lora.registerPSendCADCallback(cad_cb);
	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);
	api.system.firmwareVersion.set("RUI3-Soil-Sensor-V1.0.0");
	Serial.println("RAKwireless RUI3 Soil Sensor");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Ver %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Register the custom AT command to get device status
	if (!init_status_at())
	{
		MYLOG("SETUP", "Add custom AT command STATUS fail");
	}

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		MYLOG("SETUP", "Add custom AT command Send Interval fail");
	}

	// Get saved sending interval from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	// Initialize the Modbus interface on Serial1 (connected to RAK5802 RS485 module)
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	Serial1.begin(4800); // baud-rate at 19200
	master.start();
	master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over

	// Create a timer for interval reading of sensor from Modbus slave.
	api.system.timer.create(RAK_TIMER_0, modbus_start_sensor, RAK_TIMER_PERIODIC);
	// Start a timer.
	api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);

	// Create a timer for handling downlink write request to Modbus slave.
	api.system.timer.create(RAK_TIMER_1, modbus_write_coil, RAK_TIMER_ONESHOT);

	// Create a timer to read the sensor after 30 seconds power up.
	api.system.timer.create(RAK_TIMER_2, modbus_read_register, RAK_TIMER_ONESHOT);

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		modbus_start_sensor(NULL);
	}
	else
	{
		digitalWrite(WB_IO2, LOW);
	}

	if (api.lorawan.nwm.get() == 1)
	{
		if (g_confirmed_mode)
		{
			MYLOG("SETUP", "Confirmed enabled");
		}
		else
		{
			MYLOG("SETUP", "Confirmed disabled");
		}

		MYLOG("SETUP", "Retry = %d", g_confirmed_retry);

		MYLOG("SETUP", "DR = %d", g_data_rate);
	}

	// Enable low power mode
	api.system.lpm.set(1);

	// If available, enable BLE advertising for 30 seconds and open the BLE UART channel
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
// No BLE
#else
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.advertise.start(30);
#endif
}

void modbus_start_sensor(void *)
{
	digitalWrite(WB_IO2, HIGH);
	digitalWrite(LED_BLUE, HIGH);
	MYLOG("MODR", "Power-up sensor");
	api.system.timer.start(RAK_TIMER_2, SENSOR_POWER_TIME, NULL); // 600000 ms = 600 seconds = 10 minutes power on
}

void modbus_read_register(void *)
{
	MYLOG("MODR", "Send read request over ModBus");
	coils_n_regs.data[0] = coils_n_regs.data[1] = coils_n_regs.data[2] = coils_n_regs.data[3] = coils_n_regs.data[4] = 0xFFFF;
	coils_n_regs.data[5] = coils_n_regs.data[6] = coils_n_regs.data[7] = coils_n_regs.data[8] = 0xFFFF;
	telegram.u8id = 1;					   // slave address
	telegram.u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	telegram.u16RegAdd = 0;				   // start address in slave
	telegram.u16CoilsNo = 9;			   // number of elements (coils or registers) to read
	telegram.au16reg = coils_n_regs.data;  // pointer to a memory array in the Arduino

	master.query(telegram); // send query (only once)

	time_t start_poll = millis();

	bool data_ready = false;
	while ((millis() - start_poll) < 5000)
	{
		master.poll(); // check incoming messages
		if (master.getState() == COM_IDLE)
		{
			if ((coils_n_regs.data[0] == 0xFFFF) && (coils_n_regs.data[1] == 0xFFFF) && (coils_n_regs.data[2] == 0xFFFF) && (coils_n_regs.data[3] == 0xFFFF) && (coils_n_regs.data[4] == 0xFFFF) && (coils_n_regs.data[5] == 0xFFFF) && (coils_n_regs.data[6] == 0xFFFF) && (coils_n_regs.data[7] == 0xFFFF) && (coils_n_regs.data[8] == 0xFFFF))
			{
				MYLOG("MODR", "No data received");
				MYLOG("MODR", "%04X %04X %04X %04X %04X %04X %04X %04X %04X ",
					  coils_n_regs.data[0], coils_n_regs.data[1], coils_n_regs.data[2], coils_n_regs.data[3],
					  coils_n_regs.data[4], coils_n_regs.data[5], coils_n_regs.data[6], coils_n_regs.data[7], coils_n_regs.data[8]);
				break;
			}
			else
			{
				MYLOG("MODR", "Moisture = %.2f", coils_n_regs.sensor_data.moisture / 10.0);
				MYLOG("MODR", "Temperature = %.2f", coils_n_regs.sensor_data.temperature / 10.0);
				MYLOG("MODR", "Conductivity = %.1f", coils_n_regs.sensor_data.conductivity * 1.0);
				MYLOG("MODR", "pH = %.2f", coils_n_regs.sensor_data.ph_value / 10.0);
				MYLOG("MODR", "Nitrogen = %.2f", coils_n_regs.sensor_data.nitrogen * 1.0);
				MYLOG("MODR", "Phosphorus = %.2f", coils_n_regs.sensor_data.phosphorus * 1.0);
				MYLOG("MODR", "Potassium = %.2f", coils_n_regs.sensor_data.potassium * 1.0);
				MYLOG("MODR", "Salinity = %.2f", coils_n_regs.sensor_data.salinity * 1.0);
				MYLOG("MODR", "TDS = %.2f", coils_n_regs.sensor_data.tds * 1.0);

				data_ready = true;

				// Clear payload
				g_solution_data.reset();

				// if (coils_n_regs.sensor_data.temperature != 0)
				{
					g_solution_data.addTemperature(LPP_CHANNEL_TEMP, coils_n_regs.sensor_data.temperature / 10.0);
				}
				// if (coils_n_regs.sensor_data.moisture != 0)
				{
					g_solution_data.addRelativeHumidity(LPP_CHANNEL_MOIST, coils_n_regs.sensor_data.moisture / 10.0);
				}
				// if (coils_n_regs.sensor_data.conductivity != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_COND, coils_n_regs.sensor_data.conductivity * 1.0);
				}
				// if (coils_n_regs.sensor_data.ph_value != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_PH, coils_n_regs.sensor_data.ph_value / 10.0);
				}
				// if (coils_n_regs.sensor_data.nitrogen != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_NITRO, coils_n_regs.sensor_data.nitrogen * 1.0);
				}
				// if (coils_n_regs.sensor_data.phosphorus != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_PHOS, coils_n_regs.sensor_data.phosphorus * 1.0);
				}
				// if (coils_n_regs.sensor_data.potassium != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_POTA, coils_n_regs.sensor_data.potassium * 1.0);
				}
				// if (coils_n_regs.sensor_data.salinity != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_SALIN, coils_n_regs.sensor_data.salinity * 1.0);
				}
				// if (coils_n_regs.sensor_data.tds != 0)
				{
					g_solution_data.addAnalogInput(LPP_CHANNEL_TDS, coils_n_regs.sensor_data.tds * 1.0);
				}

				float battery_reading = 0.0;
				// Add battery voltage
				for (int i = 0; i < 10; i++)
				{
					battery_reading += api.system.bat.get(); // get battery voltage
				}

				battery_reading = battery_reading / 10;

				g_solution_data.addVoltage(LPP_CHANNEL_BATT, battery_reading);

				break;
			}
		}
	}

	if (data_ready)
	{
		// Send the packet
		send_packet();
	}
	digitalWrite(WB_IO2, LOW);
	digitalWrite(LED_BLUE, LOW);
}

void modbus_write_coil(void *)
{
	// Coils are in 16 bit register in form of 7-0, 15-8
	digitalWrite(WB_IO2, HIGH);
	MYLOG("MODW", "Send write coil request over ModBus");

	MYLOG("MODW", "Num of coils %d", coil_data.num_coils);

	// Reset the register
	coils_n_regs.data[0] = 0;

	// Prepare coils STATUS
	uint8_t coil_shift = 8;
	for (int idx = 0; idx < coil_data.num_coils; idx++)
	{
		MYLOG("MODW", "Coil %d %s %d", idx, coil_data.coils[idx] == 0 ? "off" : "on", coil_data.coils[idx] << coil_shift);
		coils_n_regs.data[0] |= coil_data.coils[idx] << coil_shift;
		MYLOG("MODW", "Coil data %02X", coils_n_regs.data[0]);
		coil_shift++;
		if (coil_shift == 16)
		{
			coil_shift = 0;
		}
	}
	MYLOG("MODW", "Coil data %02X", coils_n_regs.data[0]);

	telegram.u8id = coil_data.dev_addr;			 // slave address
	telegram.u8fct = MB_FC_WRITE_MULTIPLE_COILS; // function code (this one is coil write)
	telegram.u16RegAdd = 0;						 // start address in slave
	telegram.u16CoilsNo = coil_data.num_coils;	 // number of elements (coils or registers) to write
	telegram.au16reg = coils_n_regs.data;		 // pointer to a memory array in the Arduino

	master.query(telegram); // send query (only once)

	time_t start_poll = millis();

	while ((millis() - start_poll) < 5000)
	{
		master.poll(); // check incoming messages
		if (master.getState() == COM_IDLE)
		{
			MYLOG("MODW", "Write done");
			break;
		}
	}

	digitalWrite(WB_IO2, LOW);
}

/**
 * @brief This example is complete timer driven.
 * The loop() does nothing than sleep.
 *
 */
void loop(void)
{
	api.system.sleep.all();
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		MYLOG("UPLINK", "Sending packet over LoRaWAN with size %d", g_solution_data.getSize());
		uint8_t proposed_dr = get_min_dr(api.lorawan.band.get(), g_solution_data.getSize());
		MYLOG("UPLINK", "Check if datarate allows payload size, proposed is DR %d, current DR is %d", proposed_dr, api.lorawan.dr.get());

		if (proposed_dr == 16)
		{
			MYLOG("UPLINK", "No matching DR found");
		}
		else
		{
			if (proposed_dr < api.lorawan.dr.get())
			{
				MYLOG("UPLINK", "Proposed DR is lower than current selected, switching to lower DR");
				api.lorawan.dr.set(proposed_dr);
			}

			if (proposed_dr > api.lorawan.dr.get())
			{
				MYLOG("UPLINK", "Proposed DR is higher than current selected, switching to higher DR");
				api.lorawan.dr.set(proposed_dr);
			}
		}

		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "Packet enqueued, size %d", g_solution_data.getSize());
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
	// It is P2P
	else
	{
		MYLOG("UPLINK", "Send packet with size %d over P2P", g_solution_data.getSize());

		digitalWrite(LED_BLUE, LOW);

		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}

