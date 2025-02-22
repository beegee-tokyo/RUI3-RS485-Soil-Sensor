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

#include "app.h"

// VEM SEE SN-3002-TR-ECTHNPKKPH-N01
#define VEMSEE
// GEMHO 7in1 Soil Sensor with RS485
// #define GEMHO

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

/** Coils structure */
coil_s coil_data;

/** Register structure */
register_s register_data;

/** Flag if write is for coils or for registers */
bool is_registers = false;

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

/** Flag if sensor reading is active */
bool sensor_active = false;

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
	String version = "RUI3-Soil-Sensor-V" + String(SW_VERSION_0) + "." + String(SW_VERSION_1) + "." + String(SW_VERSION_2);
	String module_type = api.system.modelId.get();
	module_type.toUpperCase();
	api.system.firmwareVersion.set(version);
	Serial.println("RAKwireless RUI3 Soil Sensor");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("App Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.printf("BSP Version %s\n", sw_version);
	Serial.println("------------------------------------------------------");

	// Register the custom AT command to get device status
	if (!init_status_at())
	{
		MYLOG("SETUP", "Add custom AT command STATUS failed");
	}

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		MYLOG("SETUP", "Add custom AT command Send Interval failed");
	}

	// Register sensor test command
	if (!init_test_at())
	{
		MYLOG("SETUP", "Add custom AT command sensor test failed");
	}

	// Get saved sending interval from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	// Initialize the Modbus interface on Serial1 (connected to RAK5802 RS485 module)
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	Serial1.end();
#ifdef VEMSEE
	Serial1.begin(4800, RAK_CUSTOM_MODE);
#endif
#ifdef GEMHO
	Serial1.begin(9600, RAK_CUSTOM_MODE);
#endif
	// master.start();
	// master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over

	// Create a timer for interval reading of sensor from Modbus slave.
	api.system.timer.create(RAK_TIMER_0, modbus_start_sensor, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}

	// Create a timer for handling downlink write request to Modbus slave.
	api.system.timer.create(RAK_TIMER_1, modbus_write_coil, RAK_TIMER_ONESHOT);

	// Create a timer to read the sensor after 30 seconds power up.
	api.system.timer.create(RAK_TIMER_2, modbus_read_register, RAK_TIMER_ONESHOT);

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);
		MYLOG("SETUP", "P2P mode, start a reading");
		modbus_start_sensor(NULL);
	}
	else
	{
		// Shut down 12V supply and RS485
		digitalWrite(WB_IO2, LOW);
		Serial1.end();
		udrv_serial_deinit(SERIAL_UART1);
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

/**
 * @brief Power up sensor for data collection
 * 		Power up time is defined by SENSOR_POWER_TIME
 * 		Sensor reading and data transmission is done after SENSOR_POWER_TIME
 *
 */
void modbus_start_sensor(void *)
{
	digitalWrite(WB_IO2, HIGH);
	digitalWrite(LED_BLUE, HIGH);
	sensor_active = true;
	MYLOG("MODR", "Power-up sensor");
	api.system.timer.start(RAK_TIMER_2, SENSOR_POWER_TIME, NULL); // 600000 ms = 600 seconds = 10 minutes power on
}

/**
 * @brief Read ModBus registers
 * 		Reads first 9 registers with the sensor data
 * 		If sensor data could be retrieved, the sensor data is sent over LoRa/LoRaWAN
 *
 */
void modbus_read_register(void *test)
{
	if (test != NULL)
	{
		MYLOG("MODR", "Test sensor reading");
	}
	else
	{
		MYLOG("MODR", "Scheduled sensor reading");
	}
	time_t start_poll;
	bool data_ready;
#ifdef VEMSEE
	Serial1.begin(4800, RAK_CUSTOM_MODE);
#endif
#ifdef GEMHO
	Serial1.begin(9600, RAK_CUSTOM_MODE);
#endif
	MYLOG("MODR", "Serial initialized");
	delay(500);
	master.start();
	master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over
	MYLOG("MODR", "Modbus master initialized");
	delay(500);

	// Clear payload
	g_solution_data.reset();

#ifdef VEMSEE
	MYLOG("MODR", "Send read request over ModBus");
	// Clear data structure
	coils_n_regs.data[0] = coils_n_regs.data[1] = coils_n_regs.data[2] = coils_n_regs.data[3] = coils_n_regs.data[4] = 0xFFFF;
	coils_n_regs.data[5] = coils_n_regs.data[6] = coils_n_regs.data[7] = coils_n_regs.data[8] = 0xFFFF;
	// Setup read command
	telegram.u8id = 1;					   // slave address
	telegram.u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	telegram.u16RegAdd = 0;				   // start address in slave
	telegram.u16CoilsNo = 9;			   // number of elements (coils or registers) to read
	telegram.au16reg = coils_n_regs.data;  // pointer to a memory array in the Arduino

	// Send query (only once)
	master.query(telegram);

	start_poll = millis();
	data_ready = false;
	// Wait for slave response for 5 seconds
	while ((millis() - start_poll) < 10000)
	{
		// Check incoming messages
		if (master.poll() != 0)
		{
			// Status idle, either data was received or the slave response timed out
			if (master.getState() == COM_IDLE)
			{
				// Check if register structure has changed
				// if (coils_n_regs.data[0] == 0xFFFF)
				if ((uint16_t)(coils_n_regs.sensor_data.reg_1) == 0xFFFF)
				{
					MYLOG("MODR", "No data received");
					MYLOG("MODR", "%04X %04X %04X %04X %04X %04X %04X %04X %04X ",
						  coils_n_regs.data[0], coils_n_regs.data[1], coils_n_regs.data[2], coils_n_regs.data[3],
						  coils_n_regs.data[4], coils_n_regs.data[5], coils_n_regs.data[6], coils_n_regs.data[7], coils_n_regs.data[8]);
					break;
				}
				else
				{
					MYLOG("MODR", "Moisture = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_1) / 10.0);
					MYLOG("MODR", "Temperature = %.2f", coils_n_regs.sensor_data.reg_2 / 10.0);
					MYLOG("MODR", "Conductivity = %.1f", (uint16_t)coils_n_regs.sensor_data.reg_3 * 1.0);
					MYLOG("MODR", "pH = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_4) / 10.0);
					MYLOG("MODR", "Nitrogen = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_5) * 1.0);
					MYLOG("MODR", "Phosphorus = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_6) * 1.0);
					MYLOG("MODR", "Potassium = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_7) * 1.0);
					MYLOG("MODR", "Salinity = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_8) * 1.0);
					MYLOG("MODR", "TDS = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_9) * 1.0);

					data_ready = true;

					// Add temperature level to payload
					g_solution_data.addTemperature(LPP_CHANNEL_TEMP, coils_n_regs.sensor_data.reg_2 / 10.0);

					// Add moisture level to payload
					g_solution_data.addRelativeHumidity(LPP_CHANNEL_MOIST, (uint16_t)(coils_n_regs.sensor_data.reg_1) / 10.0);

					// Add conductivity value to payload
					g_solution_data.addConcentration(LPP_CHANNEL_COND, (uint16_t)(coils_n_regs.sensor_data.reg_3));

					// Add pH value to payload
					g_solution_data.addAnalogOutput(LPP_CHANNEL_PH, (uint16_t)(coils_n_regs.sensor_data.reg_4) / 10);

					// Add nitrogen level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_NITRO, (uint16_t)(coils_n_regs.sensor_data.reg_5));

					// Add phosphorus level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_PHOS, (uint16_t)(coils_n_regs.sensor_data.reg_6));

					// Addf potassium level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_POTA, (uint16_t)(coils_n_regs.sensor_data.reg_7));

					// Add salinity level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_SALIN, (uint16_t)(coils_n_regs.sensor_data.reg_8));

					// Add TDS value to payload
					g_solution_data.addConcentration(LPP_CHANNEL_TDS, (uint16_t)(coils_n_regs.sensor_data.reg_9));

					break;
				}
			}
		}
	}
#endif

#ifdef GEMHO
	MYLOG("MODR", "Send read request over ModBus");
	// Clear data structure
	coils_n_regs.data[0] = coils_n_regs.data[1] = coils_n_regs.data[2] = coils_n_regs.data[3] = coils_n_regs.data[4] = 0xFFFF;
	coils_n_regs.data[5] = coils_n_regs.data[6] = coils_n_regs.data[7] = coils_n_regs.data[8] = 0xFFFF;

	// Setup read command for T, H, E and pH
	telegram.u8id = 1;					   // slave address
	telegram.u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	telegram.u16RegAdd = 6;				   // start address in slave
	telegram.u16CoilsNo = 4;			   // number of elements (coils or registers) to read
	telegram.au16reg = coils_n_regs.data;  // pointer to a memory array in the Arduino

	// Send query (only once)
	master.query(telegram);

	start_poll = millis();
	data_ready = false;
	// Wait for slave response for 10 seconds
	while ((millis() - start_poll) < 10000)
	{
		// Check incoming messages
		if (master.poll() != 0)
		{
			// Status idle, either data was received or the slave response timed out
			if (master.getState() == COM_IDLE)
			{
				// Check if register structure has changed
				if ((coils_n_regs.data[0] == 0xFFFF) && (coils_n_regs.data[1] == 0xFFFF) && (coils_n_regs.data[2] == 0xFFFF) && (coils_n_regs.data[3] == 0xFFFF))
				{
					MYLOG("MODR", "No data received");
					MYLOG("MODR", "%04X %04X %04X %04X",
						  coils_n_regs.data[0], coils_n_regs.data[1], coils_n_regs.data[2], coils_n_regs.data[3]);
					break;
				}
				else
				{
					MYLOG("MODR", "Moisture = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_2) / 100.0);
					MYLOG("MODR", "Temperature = %.2f", coils_n_regs.sensor_data.reg_1 / 100.0);
					MYLOG("MODR", "Conductivity = %.1f", (uint16_t)coils_n_regs.sensor_data.reg_3);
					MYLOG("MODR", "pH = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_4) / 100.0);
					data_ready = true;

					// Add temperature level to payload
					g_solution_data.addTemperature(LPP_CHANNEL_TEMP, coils_n_regs.sensor_data.reg_2 / 100);

					// Add moisture level to payload
					g_solution_data.addRelativeHumidity(LPP_CHANNEL_MOIST, (uint16_t)(coils_n_regs.sensor_data.reg_1) / 100);

					// Add conductivity value to payload
					g_solution_data.addConcentration(LPP_CHANNEL_COND, (uint16_t)(coils_n_regs.sensor_data.reg_3));

					// Add pH value to payload
					g_solution_data.addAnalogOutput(LPP_CHANNEL_PH, (uint16_t)(coils_n_regs.sensor_data.reg_4) / 100);

					break;
				}
			}
		}
	}

	// Clear data structure
	coils_n_regs.data[0] = coils_n_regs.data[1] = coils_n_regs.data[2] = 0xFFFF;

	// Setup read command for N, Ph, Po
	telegram.u8id = 1;					   // slave address
	telegram.u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	telegram.u16RegAdd = 0x1e;			   // start address in slave
	telegram.u16CoilsNo = 3;			   // number of elements (coils or registers) to read
	telegram.au16reg = coils_n_regs.data;  // pointer to a memory array in the Arduino

	// Send query (only once)
	master.query(telegram);

	start_poll = millis();
	// Wait for slave response for 10 seconds
	while ((millis() - start_poll) < 10000)
	{
		// Check incoming messages
		if (master.poll() != 0)
		{
			// Status idle, either data was received or the slave response timed out
			if (master.getState() == COM_IDLE)
			{
				// Check if register structure has changed
				if ((coils_n_regs.data[0] == 0xFFFF) && (coils_n_regs.data[1] == 0xFFFF) && (coils_n_regs.data[2] == 0xFFFF))
				{
					MYLOG("MODR", "No data received");
					MYLOG("MODR", "%04X %04X %04X",
						  coils_n_regs.data[0], coils_n_regs.data[1], coils_n_regs.data[2]);
					data_ready = false;
					break;
				}
				else
				{
					MYLOG("MODR", "Nitrogen = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_1));
					MYLOG("MODR", "Phosphorus = %.2f", (uint16_t)(coils_n_regs.sensor_data.reg_2));
					MYLOG("MODR", "Potatium = %.1f", (uint16_t)(coils_n_regs.sensor_data.reg_3));
					data_ready = true;

					// Add nitrogen level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_NITRO, (uint16_t)(coils_n_regs.sensor_data.reg_1));

					// Add phosphorus level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_PHOS, (uint16_t)(coils_n_regs.sensor_data.reg_2));

					// Addf potassium level to payload
					g_solution_data.addConcentration(LPP_CHANNEL_POTA, (uint16_t)(coils_n_regs.sensor_data.reg_3));

					break;
				}
			}
		}
	}
#endif

	if (test != NULL)
	{
		if (data_ready)
		{
			AT_PRINTF("+EVT:Sensor Values: M:%.2f-T:%.2f-pH:%.2f-C:%.1f\r\n", (uint16_t)(coils_n_regs.sensor_data.reg_1) / 10.0,
					  (uint16_t)(coils_n_regs.sensor_data.reg_2) / 10.0,
					  (uint16_t)(coils_n_regs.sensor_data.reg_4) / 10.0,
					  (uint16_t)(coils_n_regs.sensor_data.reg_3) * 1.0);
		}
		else
		{
			AT_PRINTF("+EVT:Error reading sensor\r\n");
		}
		return;
	}

	// Shut down sensors and communication for lowest power consumption
	digitalWrite(WB_IO2, LOW);
	Serial1.end();
	udrv_serial_deinit(SERIAL_UART1);
	digitalWrite(LED_BLUE, LOW);
	sensor_active = false;

	// Add battery voltage
	float battery_reading = 0.0;

	for (int i = 0; i < 10; i++)
	{
		battery_reading += api.system.bat.get(); // get battery voltage
	}

	battery_reading = battery_reading / 10;

	g_solution_data.addVoltage(LPP_CHANNEL_BATT, battery_reading);

	if (data_ready)
	{
		// Report no error
		g_solution_data.addDigitalInput(LPP_CHANNEL_ERROR, 0);

		// Send the packet if data was received
		send_packet();
	}
	else
	{
		// Report error
		g_solution_data.addDigitalInput(LPP_CHANNEL_ERROR, 1);
		// Send the packet if data was received
		send_packet();
	}
}

/**
 * @brief Write to ModBus slave
 * 		Modbus register/coil address and data is prepared in
 * 		coil_data structure or registers_data structure
 *
 */
void modbus_write_coil(void *)
{
	// Coils are in 16 bit register in form of 7-0, 15-8
	digitalWrite(WB_IO2, HIGH);
#ifdef VEMSEE
	Serial1.begin(4800, RAK_CUSTOM_MODE);
#endif
#ifdef GEMHO
	Serial1.begin(9600, RAK_CUSTOM_MODE);
#endif

	// Check if we write coils or registers
	if (is_registers)
	{
		MYLOG("MODW", "Send write register request over ModBus");
		MYLOG("MODW", "Num of registers %d", register_data.num_registers);

		coils_n_regs.data[0] = coils_n_regs.data[1] = coils_n_regs.data[2] = coils_n_regs.data[3] = 0;
		coils_n_regs.data[4] = coils_n_regs.data[5] = coils_n_regs.data[6] = coils_n_regs.data[7] = 0;

		// Check number of registers to write
		if (register_data.num_registers > 8)
		{
			MYLOG("MODW", "Too many registers requested to write. Only max 8 are allowed");
			return;
		}
		// Save register status
		for (int idx = 0; idx < register_data.num_registers; idx++)
		{
			coils_n_regs.data[idx] = register_data.registers[idx];
		}

		telegram.u8id = register_data.dev_addr; // slave address
		if (register_data.num_registers == 1)
		{
			telegram.u8fct = MB_FC_WRITE_REGISTER; // function code (this one is single register write)
		}
		else
		{
			telegram.u8fct = MB_FC_WRITE_MULTIPLE_REGISTERS; // function code (this one is multiple registers write write)
		}
		telegram.u16RegAdd = register_data.register_start_address; // start address in slave
		telegram.u16CoilsNo = register_data.num_registers;		   // number of registers to write
		telegram.au16reg = coils_n_regs.data;					   // pointer to a memory array in the Arduino
	}
	else
	{
		MYLOG("MODW", "Send write coil request over ModBus");
		MYLOG("MODW", "Num of coils %d", coil_data.num_coils);

		// Reset the register
		coils_n_regs.data[0] = 0;

		// Check number of coils to write
		if (coil_data.num_coils > 16)
		{
			MYLOG("MODW", "Too many coils requested to write. Only max 16 are allowed");
			return;
		}
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
		telegram.u16CoilsNo = coil_data.num_coils;	 // number of coils to write
		telegram.au16reg = coils_n_regs.data;		 // pointer to a memory array in the Arduino
	}
	// Send query (only once)
	master.query(telegram);

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

	// Shut down sensors and communication for lowest power consumption
	digitalWrite(WB_IO2, LOW);
	Serial1.end();
	udrv_serial_deinit(SERIAL_UART1);
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
 * Cayenne LPP format by the different sensor functions
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
