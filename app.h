/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.1
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <Arduino.h>
#include "RUI3_ModbusRtu.h"

// Test mode
// Test mode set to 0 to use long sensor reading times
#ifndef TEST_MODE
#define TEST_MODE 0
#endif

#if TEST_MODE == 1
// Sensor reading time (how long sensor is powered up before data is read)
#define SENSOR_POWER_TIME (60000) // 1 minute
#else
// Sensor reading time (how long sensor is powered up before data is read)
#define SENSOR_POWER_TIME (300000) // 5 minutes
#endif

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

#if MY_DEBUG > 0
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
		Serial.flush();                  \
	} while (0);                         \
	delay(100)
#else // RAK4630 || RAK11720
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\r\n");           \
		Serial6.printf(__VA_ARGS__);     \
		Serial6.printf("\r\n");          \
		Serial.flush();                  \
	} while (0);                         \
	delay(100)
#endif
#else
#define MYLOG(...)
#endif

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define AT_PRINTF(...)              \
	do                              \
	{                               \
		Serial.printf(__VA_ARGS__); \
		Serial.printf("\r\n");      \
	} while (0);                    \
	delay(100)
#else // RAK4630 || RAK11720
#define AT_PRINTF(...)               \
	do                               \
	{                                \
		Serial.printf(__VA_ARGS__);  \
		Serial.printf("\r\n");       \
		Serial6.printf(__VA_ARGS__); \
		Serial6.printf("\r\n");      \
	} while (0);                     \
	delay(100)
#endif

// Modbus stuff
/** Sensor register structure */
struct sensor_data_s
{
	// int16_t coils;
	int16_t reg_1;
	int16_t reg_2;
	int16_t reg_3;
	int16_t reg_4;
	int16_t reg_5;
	int16_t reg_6;
	int16_t reg_7;
	int16_t reg_8;
	int16_t reg_9;
};

/** Union for received data mapping */
union coils_n_regs_u
{
	sensor_data_s sensor_data;
	int16_t data[9];
};

extern coils_n_regs_u au16data;

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

/** This is the structure which contains a write to set/reset coils */
struct coil_s
{
	int8_t dev_addr = 1;
	int8_t coils[16];
	int8_t num_coils = 0;
};

/** This is the structure to write to specific registers in the ModBus slave */
struct register_s
{
	int8_t dev_addr = 1;
	int8_t registers[16];
	int8_t num_registers = 0;
	int16_t register_start_address = 0;
};

// Forward declarations
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool init_test_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
void joinCallback(int32_t status);
void receiveCallback(SERVICE_LORA_RECEIVE_T *data);
void sendCallback(int32_t status);
void recv_cb(rui_lora_p2p_recv_t data);
void send_cb(void);
void cad_cb(bool result);
void modbus_read_register(void *test);
extern bool is_registers;
extern coil_s coil_data;
extern register_s register_data;
extern bool sensor_active;
extern const char *sw_version;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1 // Base Board
#define LPP_CHANNEL_MOIST 2
#define LPP_CHANNEL_TEMP 3
#define LPP_CHANNEL_COND 4
#define LPP_CHANNEL_PH 5
#define LPP_CHANNEL_NITRO 6
#define LPP_CHANNEL_PHOS 7
#define LPP_CHANNEL_POTA 8
#define LPP_CHANNEL_SALIN 9
#define LPP_CHANNEL_TDS 10
#define LPP_CHANNEL_ERROR 11

extern WisCayenne g_solution_data;
