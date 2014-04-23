
/* global variables definition */
extern int32_t g_engine_id;
extern int32_t g_serial_baud;
extern int32_t g_status_period;
extern float g_vbat_vd1_voltage_drop;
extern int32_t g_batt_cells;
extern char g_batt_type[PT_STRING_SIZE];

/* global callbacks */
extern void on_serial1_change(const struct param_entry *p ATTR_UNUSED);
extern void on_batt_type_change(const struct param_entry *p ATTR_UNUSED);

/* special variables for host system (for identifying ECU) */
static char l_engine_name[PT_STRING_SIZE];
static char l_engine_serial_no[PT_STRING_SIZE];
static char l_ecu_serial_no[PT_STRING_SIZE];

static const struct param_entry parameter_table[] = {
	PARAM_STRING("ENGINE_NAME", l_engine_name, "mfg & name", NULL),
	PARAM_STRING("ENGINE_SERIAL", l_engine_serial_no, "serial no", NULL),
	PARAM_STRING("ECU_SERIAL", l_ecu_serial_no, "TBD", NULL), // TODO make it read only
	PARAM_INT32("ENGINE_ID", g_engine_id, 1, 1, 255, NULL),
	PARAM_INT32("SERIAL1_BAUD", g_serial_baud, 57600, 9600, 115200, on_serial1_change),
	PARAM_INT32("STATUS_PERIOD", g_status_period, 1000, 100, 60000, NULL),
	PARAM_FLOAT("VBAT_VD1_VD", g_vbat_vd1_voltage_drop, 0.450, 0.0, 1.0, NULL),
	PARAM_INT32("BATT_CELLS", g_batt_cells, 4, 1, 10, NULL),
	PARAM_STRING("BATT_TYPE", g_batt_type, "NiMH", on_batt_type_change)
};

