
/* global variables definition */
extern int32_t g_engine_id;
extern int32_t g_serial_baud;
extern int32_t g_status_period;
extern float g_vbat_vd1_voltage_drop;
extern int32_t g_batt_cells;
extern char g_batt_type[PT_STRING_SIZE];
extern int32_t g_temp_r;
extern float g_temp_overheat;
extern float g_temp_sh_a;
extern float g_temp_sh_b;
extern float g_temp_sh_c;
extern bool g_debug_enable_adc_raw;
extern bool g_debug_enable_memdump;
extern int32_t g_pulses_per_revolution;
extern int32_t g_rpm_limit;
extern int32_t g_rpm_min_idle;

/* global callbacks */
extern void on_serial1_change(const struct param_entry *p ATTR_UNUSED);
extern void on_batt_type_change(const struct param_entry *p ATTR_UNUSED);

/* special variables for host system (for identifying ECU) */
static char l_engine_name[PT_STRING_SIZE];
static char l_engine_serial_no[PT_STRING_SIZE];
static char l_ecu_serial_no[PT_STRING_SIZE];

static void on_ecu_serial_no_change(const struct param_entry *p ATTR_UNUSED)
{
#define STM32F37x_UID_BASE	0x1FFFF7AC
	uint32_t uid0 = *((vuc32 *) (STM32F37x_UID_BASE + 0x00));
	uint32_t uid1 = *((vuc32 *) (STM32F37x_UID_BASE + 0x04));
	uint32_t uid2 = *((vuc32 *) (STM32F37x_UID_BASE + 0x08));

	/* same number as calculated in USB DFU bootloader
	 * See @a https://my.st.com/public/STe2ecommunities/mcu/Tags.aspx?tags=id%20unique%20meaning%20shortening
	 */
	uint32_t sn_hi = (((uid2 >> 24) & 0xff) << 8) |
		(((uid2 >> 16) & 0xff) + ((uid0 >> 16) & 0xff));

	uint32_t sn_lo = (((uid2 >> 8) & 0xff) << 24) |
		(((uid2 & 0xff) + (uid0 & 0xff)) << 16) |
		(((uid1 >> 24) & 0xff) << 8) |	/* following two values don't match to DFU */
		(((uid1 >> 16) & 0xff));	/* it was uid2[31:24] and uid2[23:16] */

	chsnprintf(l_ecu_serial_no, PT_STRING_SIZE, "SN%04x%08x", sn_hi, sn_lo);
}

static const struct param_entry parameter_table[] = {
	// @DESC: engine manufacturer and model
	PARAM_STRING("ENGINE_NAME", l_engine_name, "mfg & name", NULL),
	// @DESC: engine serial number
	PARAM_STRING("ENGINE_SERIAL", l_engine_serial_no, "serial no", NULL),
	// @DESC: ECU address
	PARAM_INT32("ENGINE_ID", g_engine_id, 1, 1, 255, NULL),

	// @DESC: ECU module serial number
	// @READ-ONLY
	PARAM_STRING("ECU_SERIAL", l_ecu_serial_no, "SETUP ERROR", on_ecu_serial_no_change),

	// @DESC: baud rate for UART
	// @VALUES: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600
	PARAM_INT32("SERIAL1_BAUD", g_serial_baud, 57600, 9600, 921600, on_serial1_change),
	// @DESC: period of Status message in milliseconds
	PARAM_INT32("STATUS_PERIOD", g_status_period, 1000, 100, 60000, NULL),

	// @DESC: Voltage drop on VD1 diode (for compensation)
	PARAM_FLOAT("BATT_VD1_VD", g_vbat_vd1_voltage_drop, 0.400, 0.0, 1.0, NULL),
	// @DESC: number of cells in battery
	PARAM_INT32("BATT_CELLS", g_batt_cells, 4, 1, 10, NULL),
	// @DESC: battery chemistry type
	// @VALUES: NiMH, NiCd, LiIon, LiPo, LiFePo, Pb
	PARAM_STRING("BATT_TYPE", g_batt_type, "NiMH", on_batt_type_change),

	// @DESC: TEMP input mode R1 or R2
	// @ENUM: R1=1, R2=2
	PARAM_INT32("TEMP_R", g_temp_r, 2, 1, 2, NULL),
	// @DESC: engine overheat temperature
	PARAM_FLOAT("TEMP_OVERHEAT", g_temp_overheat, 110.0, 0, 200, NULL),
	// @DESC: Steinhart-Hart A koeff for TEMP
	PARAM_FLOAT("TEMP_SH_A", g_temp_sh_a, 2.1085e-3, -10, 10, NULL),
	// @DESC: Steinhart-Hart B koeff for TEMP
	PARAM_FLOAT("TEMP_SH_B", g_temp_sh_b, 0.7979e-4, -10, 10, NULL),
	// @DESC: Steinhart-Hart C koeff for TEMP
	PARAM_FLOAT("TEMP_SH_C", g_temp_sh_c, 6.5351e-7, -10, 10, NULL),

	// @DESC: High RPM limit
	PARAM_INT32("RPM_LIMIT", g_rpm_limit, 8000, 0, 20000, NULL),
	// @DESC: Number of pulses per revolution of crankshaft
	PARAM_INT32("RPM_NPULSES", g_pulses_per_revolution, 1, 1, 64, NULL),
	// @DESC: Low RPM limit (Idle RPM - 10%..20%)
	PARAM_INT32("RPM_MIN_IDLE", g_rpm_min_idle, 800, 0, 3000, NULL),

	// @DESC: Enable debug feuture: send Status.adc_raw message
	PARAM_BOOL("DEBUG_ADC_RAW", g_debug_enable_adc_raw, false, NULL),
	// @DESC: Enable memdump subsystem (used for debugging)
	PARAM_BOOL("DEBUG_MEMDUMP", g_debug_enable_memdump, false, NULL)
};

