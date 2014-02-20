/*
 *   Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

#ifndef RUNTIME_H
#define RUNTIME_H

extern int start_uevent_monitor(void);
extern const char *runtime_3g_port_device(void);
extern const char *runtime_3g_port_data(void);
extern int runtime_3g_modem_model(void);

enum {
	HUAWEI_MODEM = 0,
	AMAZON_MODEM,
	UNKNOWN_MODEM,
	ZTE_MODEM,
};

enum {
	WCDMA_MODEL = 0,
	CDMA_MODEL,
	TD_SCDMA_MODEL,
	UNKNOWN_MODEL,
};

/**
 *  Access technology currently in use.
 */
enum {
    DATA_ACCESS_UNKNOWN = 0,
    DATA_ACCESS_GPRS = 1,
    DATA_ACCESS_EDGE = 2,
    DATA_ACCESS_UMTS = 3,
    DATA_ACCESS_CDMA_IS95A = 4,
    DATA_ACCESS_CDMA_IS95B = 5,
    DATA_ACCESS_CDMA_1xRTT = 6,
    DATA_ACCESS_CDMA_EvDo_0 = 7,
    DATA_ACCESS_CDMA_EvDo_A = 8,
    DATA_ACCESS_HSDPA = 9,
    DATA_ACCESS_HSUPA = 10,
    DATA_ACCESS_HSPA = 11,
    DATA_ACCESS_CDMA_EvDo_B = 12,
};

extern int current_modem_type;
void write_runtime_conf(const char * data_port, int model);

#endif
