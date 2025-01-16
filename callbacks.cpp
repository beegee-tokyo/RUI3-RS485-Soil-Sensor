/**
 * @file callbacks.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief LoRa and LoRaWAN callbacks
 * @version 0.1
 * @date 2025-01-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "app.h"

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
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
		// Check for command
		if (data->Buffer[2] == MB_FC_WRITE_MULTIPLE_COILS)
		{
			// Write coils, set flag for coils write
			is_registers = false;
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
		else if ((data->Buffer[2] == MB_FC_WRITE_REGISTER) || (data->Buffer[2] == MB_FC_WRITE_MULTIPLE_REGISTERS))
		{
			// Write registers, set flag for register write
			is_registers = true;

			// Get slave address
			register_data.dev_addr = data->Buffer[3];
			if ((register_data.dev_addr > 0) && (register_data.dev_addr < 17))
			{
				// Get start address of the registers
				register_data.register_start_address = data->Buffer[4];

				// Get number of coils
				register_data.num_registers = data->Buffer[5];

				// Save register status
				for (int idx = 0; idx < register_data.num_registers * 2; idx = idx + 2)
				{
					register_data.registers[idx / 2] = (uint16_t)(data->Buffer[6 + idx]) << 8;
					register_data.registers[idx / 2] |= (uint16_t)(data->Buffer[7 + idx]);
				}
				// Start a timer to handle the incoming register write request.
				api.system.timer.start(RAK_TIMER_1, 100, NULL);
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
