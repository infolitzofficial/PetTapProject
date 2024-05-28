/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <date_time.h>
#include "WiFi/WiFiHandler.h"
#include "System/SystemHandler.h"
#include "PacketHandler/PacketHandler.h"
#include "BMS/BMHandler.h"
#include "MC3630/AccelerometerHandler.h"

// aws
#include <modem/modem_info.h>
#include <nrf_modem.h>
#include <net/aws_iot.h>
#include <zephyr/sys/reboot.h>
#include <date_time.h>
#include <zephyr/dfu/mcuboot.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <zephyr/logging/log.h>

#include "NVS/NvsHandler.h"
#include "zephyr/sys/printk.h"

#define STACKSIZE 			2048
#define THREAD0_PRIORITY 	7
#define THREAD1_PRIORITY 	7
//aws connect work function
static struct k_work_delayable connect_work;

static bool cloud_connected = false;
static bool gnss_connected = false;
static long long llSysTick = 0;

static void GpsTask(void);
static void SystemTask(void);
const k_tid_t thread0_id;
const k_tid_t thread1_id;

K_THREAD_DEFINE(thread0_id, STACKSIZE, GpsTask, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, SystemTask, NULL, NULL, NULL,
		THREAD1_PRIORITY, 0, 0);

LOG_MODULE_REGISTER(gnss_sample, CONFIG_GNSS_SAMPLE_LOG_LEVEL);

#define PI 3.14159265358979323846
#define EARTH_RADIUS_METERS (6371.0 * 1000.0)

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static struct k_work_q gnss_work_q;

#define GNSS_WORKQ_THREAD_STACK_SIZE 2304
#define GNSS_WORKQ_THREAD_PRIORITY   5

K_THREAD_STACK_DEFINE(gnss_workq_stack_area, GNSS_WORKQ_THREAD_STACK_SIZE);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE || CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
#include "assistance.h"

static struct nrf_modem_gnss_agps_data_frame last_agps;
static struct k_work agps_data_get_work;
static volatile bool requesting_assistance;
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static struct k_work_delayable ttff_test_got_fix_work;
static struct k_work_delayable ttff_test_prepare_work;
static struct k_work ttff_test_start_work;
static uint32_t time_to_fix;
#endif

static const char update_indicator[] = {'\\', '|', '/', '-'};

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static uint64_t fix_timestamp;
static uint32_t time_blocked;

/* Reference position. */
static bool ref_used;
static double ref_latitude;
static double ref_longitude;

K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);
static K_SEM_DEFINE(time_sem, 0, 1);

static struct k_poll_event events[2] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&pvt_data_sem, 0),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&nmea_queue, 0),
};

BUILD_ASSERT(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS),
	     "CONFIG_LTE_NETWORK_MODE_LTE_M_GPS, "
	     "CONFIG_LTE_NETWORK_MODE_NBIOT_GPS or "
	     "CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS must be enabled");

BUILD_ASSERT((sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) == 1 &&
	      sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) == 1) ||
	     (sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) > 1 &&
	      sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) > 1),
	     "CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE and "
	     "CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE must be both either set or empty");

/* Returns the distance between two coordinates in meters. The distance is calculated using the
 * haversine formula.
 */
static double distance_calculate(double lat1, double lon1,
				 double lat2, double lon2)
{
	double d_lat_rad = (lat2 - lat1) * PI / 180.0;
	double d_lon_rad = (lon2 - lon1) * PI / 180.0;

	double lat1_rad = lat1 * PI / 180.0;
	double lat2_rad = lat2 * PI / 180.0;

	double a = pow(sin(d_lat_rad / 2), 2) +
		   pow(sin(d_lon_rad / 2), 2) *
		   cos(lat1_rad) * cos(lat2_rad);

	double c = 2 * asin(sqrt(a));

	return EARTH_RADIUS_METERS * c;
}

static void print_distance_from_reference(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	if (!ref_used) {
		return;
	}

	double distance = distance_calculate(pvt_data->latitude, pvt_data->longitude,
					     ref_latitude, ref_longitude);

	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)) {
		LOG_INF("Distance from reference: %.01f", distance);
	} else {
		printf("\nDistance from reference: %.01f\n", distance);
	}
}

static void gnss_event_handler(int event)
{
	int retval;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0) {
			k_sem_give(&pvt_data_sem);
		}
		break;

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	case NRF_MODEM_GNSS_EVT_FIX:
		/* Time to fix is calculated here, but it's printed from a delayed work to avoid
		 * messing up the NMEA output.
		 */
		time_to_fix = (k_uptime_get() - fix_timestamp) / 1000;
		k_work_schedule_for_queue(&gnss_work_q, &ttff_test_got_fix_work, K_MSEC(100));
		k_work_schedule_for_queue(&gnss_work_q, &ttff_test_prepare_work,
					  K_SECONDS(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL));
		break;
#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

	case NRF_MODEM_GNSS_EVT_NMEA:
		nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
		if (nmea_data == NULL) {
			LOG_ERR("Failed to allocate memory for NMEA");
			break;
		}

		retval = nrf_modem_gnss_read(nmea_data,
					     sizeof(struct nrf_modem_gnss_nmea_data_frame),
					     NRF_MODEM_GNSS_DATA_NMEA);
		if (retval == 0) {
			retval = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
		}

		if (retval != 0) {
			k_free(nmea_data);
		}
		break;

	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
		retval = nrf_modem_gnss_read(&last_agps,
					     sizeof(last_agps),
					     NRF_MODEM_GNSS_DATA_AGPS_REQ);
		if (retval == 0) {
			k_work_submit_to_queue(&gnss_work_q, &agps_data_get_work);
		}
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */
		break;

	default:
		break;
	}
}

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
K_SEM_DEFINE(lte_ready, 0, 1);

static void lte_lc_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_INF("Connected to LTE network");
			k_sem_give(&lte_ready);
		}
		break;

	default:
		break;
	}
}

void lte_connect(void)
{
	int err;

	LOG_INF("Connecting to LTE network");

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	if (err) {
		LOG_ERR("Failed to activate LTE, error: %d", err);
		return;
	}

	k_sem_take(&lte_ready, K_FOREVER);

	/* Wait for a while, because with IPv4v6 PDN the IPv6 activation takes a bit more time. */
	k_sleep(K_SECONDS(1));
}

void lte_disconnect(void)
{
	int err;

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);
	if (err) {
		LOG_ERR("Failed to deactivate LTE, error: %d", err);
		return;
	}

	LOG_INF("LTE disconnected");
}
#endif /* CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

static void agps_data_get_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL)
	/* SUPL doesn't usually provide NeQuick ionospheric corrections and satellite real time
	 * integrity information. If GNSS asks only for those, the request should be ignored.
	 */
	if (last_agps.sv_mask_ephe == 0 &&
	    last_agps.sv_mask_alm == 0 &&
	    (last_agps.data_flags & ~(NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST |
				      NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST)) == 0) {
		LOG_INF("Ignoring assistance request for only NeQuick and/or integrity");
		return;
	}
#endif /* CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL */

#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL)
	/* With minimal assistance, the request should be ignored if no GPS time or position
	 * is requested.
	 */
	if (!(last_agps.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) &&
	    !(last_agps.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST)) {
		LOG_INF("Ignoring assistance request because no GPS time or position is requested");
		return;
	}
#endif /* CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL */

	requesting_assistance = true;

	LOG_INF("Assistance data needed, ephe 0x%08x, alm 0x%08x, flags 0x%02x",
		last_agps.sv_mask_ephe,
		last_agps.sv_mask_alm,
		last_agps.data_flags);

#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	lte_connect();
#endif /* CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

	err = assistance_request(&last_agps);
	if (err) {
		LOG_ERR("Failed to request assistance data");
	}

#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	lte_disconnect();
#endif /* CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

	requesting_assistance = false;
}
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static void ttff_test_got_fix_work_fn(struct k_work *item)
{
	LOG_INF("Time to fix: %u", time_to_fix);
	if (time_blocked > 0) {
		LOG_INF("Time GNSS was blocked by LTE: %u", time_blocked);
	}
	print_distance_from_reference(&last_pvt);
	LOG_INF("Sleeping for %u seconds", CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL);
}

static int ttff_test_force_cold_start(void)
{
	int err;
	uint32_t delete_mask;

	LOG_INF("Deleting GNSS data");

	/* Delete everything else except the TCXO offset. */
	delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
		      NRF_MODEM_GNSS_DELETE_ALMANACS |
		      NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
		      NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW |
		      NRF_MODEM_GNSS_DELETE_GPS_WEEK |
		      NRF_MODEM_GNSS_DELETE_UTC_DATA |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW_PRECISION;

	/* With minimal assistance, we want to keep the factory almanac. */
	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL)) {
		delete_mask &= ~NRF_MODEM_GNSS_DELETE_ALMANACS;
	}

	err = nrf_modem_gnss_nv_data_delete(delete_mask);
	if (err) {
		LOG_ERR("Failed to delete GNSS data");
		return -1;
	}

	return 0;
}

static void ttff_test_prepare_work_fn(struct k_work *item)
{
	/* Make sure GNSS is stopped before next start. */
	nrf_modem_gnss_stop();

	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START)) {
		if (ttff_test_force_cold_start() != 0) {
			return;
		}
	}

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START)) {
		/* All A-GPS data is always requested before GNSS is started. */
		last_agps.sv_mask_ephe = 0xffffffff;
		last_agps.sv_mask_alm = 0xffffffff;
		last_agps.data_flags =
			NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST |
			NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST |
			NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST |
			NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST |
			NRF_MODEM_GNSS_AGPS_POSITION_REQUEST |
			NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST;

		k_work_submit_to_queue(&gnss_work_q, &agps_data_get_work);
	} else {
		/* Start and stop GNSS to trigger possible A-GPS data request. If new A-GPS
		 * data is needed it is fetched before GNSS is started.
		 */
		nrf_modem_gnss_start();
		nrf_modem_gnss_stop();
	}
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

	k_work_submit_to_queue(&gnss_work_q, &ttff_test_start_work);
}

static void ttff_test_start_work_fn(struct k_work *item)
{
	LOG_INF("Starting GNSS");
	if (nrf_modem_gnss_start() != 0) {
		LOG_ERR("Failed to start GNSS");
		return;
	}

	fix_timestamp = k_uptime_get();
	time_blocked = 0;
}
#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	k_sem_give(&time_sem);
}

static int modem_init(void)
{
	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		date_time_register_handler(date_time_evt_handler);
	}

	if (lte_lc_init() != 0) {
		LOG_ERR("Failed to initialize LTE link controller");
		printk("\nFailed to initialize LTE link controller");
		return -1;
	}

#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	lte_lc_register_handler(lte_lc_event_handler);
#elif !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
	lte_lc_psm_req(true);

	LOG_INF("Connecting to LTE network");

	if (lte_lc_connect() != 0) {
		LOG_ERR("Failed to connect to LTE network");
		return -1;
	}

	LOG_INF("Connected to LTE network");

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		LOG_INF("Waiting for current time");

		/* Wait for an event from the Date Time library. */
		k_sem_take(&time_sem, K_MINUTES(10));

		if (!date_time_is_valid()) {
			LOG_WRN("Failed to get current time, continuing anyway");
		}
	}
#endif

	return 0;
}

static int sample_init(void)
{
	int err = 0;

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	struct k_work_queue_config cfg = {
		.name = "gnss_work_q",
		.no_yield = false
	};

	k_work_queue_start(
		&gnss_work_q,
		gnss_workq_stack_area,
		K_THREAD_STACK_SIZEOF(gnss_workq_stack_area),
		GNSS_WORKQ_THREAD_PRIORITY,
		&cfg);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE || CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
	k_work_init(&agps_data_get_work, agps_data_get_work_fn);

	err = assistance_init(&gnss_work_q);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	k_work_init_delayable(&ttff_test_got_fix_work, ttff_test_got_fix_work_fn);
	k_work_init_delayable(&ttff_test_prepare_work, ttff_test_prepare_work_fn);
	k_work_init(&ttff_test_start_work, ttff_test_start_work_fn);
#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

	return err;
}

static int gnss_init_and_start(void)
{
#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	/* Enable GNSS. */
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0) {
		LOG_ERR("Failed to activate GNSS functional mode");
		return -1;
	}
#endif /* CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE || CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

	/* Configure GNSS. */
	if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
		LOG_ERR("Failed to set GNSS event handler");
		return -1;
	}

	/* Enable all supported NMEA messages. */
	uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK |
			     NRF_MODEM_GNSS_NMEA_GGA_MASK |
			     NRF_MODEM_GNSS_NMEA_GLL_MASK |
			     NRF_MODEM_GNSS_NMEA_GSA_MASK |
			     NRF_MODEM_GNSS_NMEA_GSV_MASK;

	if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0) {
		LOG_ERR("Failed to set GNSS NMEA mask");
		return -1;
	}

	/* This use case flag should always be set. */
	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_PERIODIC) &&
	    !IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)) {
		/* Disable GNSS scheduled downloads when assistance is used. */
		use_case |= NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
	}

	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_LOW_ACCURACY)) {
		use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
	}

	if (nrf_modem_gnss_use_case_set(use_case) != 0) {
		LOG_WRN("Failed to set GNSS use case");
	}

#if defined(CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK)
	if (nrf_modem_gnss_elevation_threshold_set(CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK) != 0) {
		LOG_ERR("Failed to set elevation threshold");
		return -1;
	}
	LOG_DBG("Set elevation threshold to %u", CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK);
#endif

#if defined(CONFIG_GNSS_SAMPLE_MODE_CONTINUOUS)
	/* Default to no power saving. */
	uint8_t power_mode = NRF_MODEM_GNSS_PSM_DISABLED;

#if defined(GNSS_SAMPLE_POWER_SAVING_MODERATE)
	power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_PERFORMANCE;
#elif defined(GNSS_SAMPLE_POWER_SAVING_HIGH)
	power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_POWER;
#endif

	if (nrf_modem_gnss_power_mode_set(power_mode) != 0) {
		LOG_ERR("Failed to set GNSS power saving mode");
		return -1;
	}
#endif /* CONFIG_GNSS_SAMPLE_MODE_CONTINUOUS */

	/* Default to continuous tracking. */
	uint16_t fix_retry = 60;
	uint16_t fix_interval = 1;

#if defined(CONFIG_GNSS_SAMPLE_MODE_PERIODIC)
	fix_retry = CONFIG_GNSS_SAMPLE_PERIODIC_TIMEOUT;
	fix_interval = CONFIG_GNSS_SAMPLE_PERIODIC_INTERVAL;
#elif defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	/* Single fix for TTFF test mode. */
	fix_retry = 0;
	fix_interval = 0;
#endif

	if (nrf_modem_gnss_fix_retry_set(fix_retry) != 0) {
		LOG_ERR("Failed to set GNSS fix retry");
		return -1;
	}

	if (nrf_modem_gnss_fix_interval_set(fix_interval) != 0) {
		LOG_ERR("Failed to set GNSS fix interval");
		return -1;
	}

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	k_work_schedule_for_queue(&gnss_work_q, &ttff_test_prepare_work, K_NO_WAIT);
#else /* !CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */
	if (nrf_modem_gnss_start() != 0) {
		LOG_ERR("Failed to start GNSS");
		return -1;
	}
#endif

	return 0;
}

static bool output_paused(void)
{
#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_LOG_LEVEL_OFF)
	return false;
#else
	return (requesting_assistance || assistance_is_active()) ? true : false;
#endif
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked   = 0;
	uint8_t in_fix    = 0;
	uint8_t unhealthy = 0;

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt_data->sv[i].sv > 0) {
			tracked++;

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) {
				in_fix++;
			}

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) {
				unhealthy++;
			}
		}
	}

	//printf("Tracking: %2d Using: %2d Unhealthy: %d\n", tracked, in_fix, unhealthy);
}

static void create_dummy_gnss(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	pvt_data->latitude= 47.6061;
	pvt_data->longitude= 122.3328;
	pvt_data->altitude = 10.07;
	pvt_data->accuracy= 80.12;
	pvt_data->speed = 45.67;
	pvt_data->speed_accuracy = 89.56;
	pvt_data->heading = 30.15;
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	printf("Latitude:       %.06f\n", pvt_data->latitude);
	printf("Longitude:      %.06f\n", pvt_data->longitude);
	printf("Altitude:       %.01f m\n", pvt_data->altitude);
	printf("Accuracy:       %.01f m\n", pvt_data->accuracy);
	printf("Speed:          %.01f m/s\n", pvt_data->speed);
	printf("Speed accuracy: %.01f m/s\n", pvt_data->speed_accuracy);
	printf("Heading:        %.01f deg\n", pvt_data->heading);
	printf("Date:           %04u-%02u-%02u\n",
	       pvt_data->datetime.year,
	       pvt_data->datetime.month,
	       pvt_data->datetime.day);
	printf("Time (UTC):     %02u:%02u:%02u.%03u\n",
	       pvt_data->datetime.hour,
	       pvt_data->datetime.minute,
	       pvt_data->datetime.seconds,
	       pvt_data->datetime.ms);
	printf("PDOP:           %.01f\n", pvt_data->pdop);
	printf("HDOP:           %.01f\n", pvt_data->hdop);
	printf("VDOP:           %.01f\n", pvt_data->vdop);
	printf("TDOP:           %.01f\n", pvt_data->tdop);
}

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

static int json_add_number(cJSON *parent, const char *str, double item)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(item);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_num);
}
#define APP_TOPICS_COUNT CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT
static int shadow_update(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	int err;
	bool version_number_include = false;
	char *message;
	int64_t message_ts = 0;
	int16_t bat_voltage = 0;
	float fTempCharger = 0.0; 
    uint16_t uPercent= 0;
	int iPetmove=0;
    MC36XX_acc_t PreAccRaw;

    memcpy(&PreAccRaw, GetMC36Data(), sizeof(MC36XX_acc_t));

	err = date_time_now(&message_ts);
	if (err) {
		LOG_ERR("date_time_now, error: %d", err);
		return err;
	}

#if defined(CONFIG_NRF_MODEM_LIB)
	/* Request battery voltage data from the modem. */
	err = modem_info_short_get(MODEM_INFO_BATTERY, &bat_voltage);
	if (err != sizeof(bat_voltage)) {
		LOG_ERR("modem_info_short_get, error: %d", err);
		return err;
	}
#endif

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || reported_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		err = -ENOMEM;
		return err;
	}

	if (version_number_include) {
		err = json_add_str(reported_obj, "app_version", CONFIG_AWS_IOT_SAMPLE_APP_VERSION);
	} else {
		err = 0;
	}

	ReadI2CPMIC(&uPercent, &fTempCharger);
	iPetmove=PetMove(PreAccRaw);
	printf("\n Latitude : %f", pvt_data->latitude);
	printf("\n Longitude : %f", pvt_data->longitude);
	//err += json_add_number(reported_obj, "Battery_voltage", bat_voltage);
	err += json_add_number(reported_obj, "ts", message_ts);
	err += json_add_number(reported_obj, "latitude", pvt_data->latitude);
	err += json_add_number(reported_obj, "longitude", pvt_data->longitude);
	err += json_add_number(reported_obj, "Percentage", uPercent);
	err += json_add_number(reported_obj, "Temp", fTempCharger);
	err += json_add_number(reported_obj, "PetMove", iPetmove);
	//err += json_add_number(reported_obj, "altitude", pvt_data->altitude);
	err += json_add_number(reported_obj, "flags", pvt_data->flags);
	//err += json_add_number(reported_obj, "speed", pvt_data->speed);
	//err += json_add_number(reported_obj, "spd_acc", pvt_data->speed_accuracy);
	err += json_add_obj(state_obj, "reported", reported_obj);
	err += json_add_obj(root_obj, "state", state_obj);

	if (err) {
		LOG_ERR("json_add, error: %d", err);
		goto cleanup;
	}

	message = cJSON_Print(root_obj);
	if (message == NULL) {
		LOG_ERR("cJSON_Print, error: returned NULL");
		err = -ENOMEM;
		goto cleanup;
	}

	char end_topic[]="sample/pet";
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = 0,
		.topic.str = end_topic,
		.topic.len = strlen(end_topic),
		.ptr = message,
		.len = strlen(message)
	};

	LOG_INF("Publishing: %s to AWS IoT broker", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		LOG_ERR("aws_iot_send, error: %d", err);
	}

	cJSON_FreeString(message);

cleanup:

	cJSON_Delete(root_obj);

	return err;
}

static int app_topics_subscribe(void)
{
	int err;
	static char custom_topic[75] = "sample/pet";
	//static char custom_topic_2[75] = "my-custom-topic/example_2";

	const struct aws_iot_topic_data topics_list[APP_TOPICS_COUNT] = {
		[0].str = custom_topic,
		[0].len = strlen(custom_topic),
		//[1].str = custom_topic_2,
		//[1].len = strlen(custom_topic_2)
	};

	err = aws_iot_subscription_topics_add(topics_list, ARRAY_SIZE(topics_list));
	if (err) {
		LOG_ERR("aws_iot_subscription_topics_add, error: %d", err);
	}

	return err;
}

static void print_received_data(const char *buf, const char *topic,
				size_t topic_len)
{
	char *str = NULL;
	cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		LOG_ERR("cJSON Parse failure");
		return;
	}

	str = cJSON_Print(root_obj);
	if (str == NULL) {
		LOG_ERR("Failed to print JSON object");
		goto clean_exit;
	}

	LOG_INF("Data received from AWS IoT console: Topic: %.*s Message: %s",
		topic_len, topic, str);

	cJSON_FreeString(str);

clean_exit:
	cJSON_Delete(root_obj);
}


static void connect_work_fn(struct k_work *work)
{
	int err;
printk("Connct to work fn");	
	if (cloud_connected) {
		return;
	}

	printk("Trying to connect to aws");
	
	err = aws_iot_connect(NULL);
	if (err) {
		LOG_ERR("aws_iot_connect, error: %d", err);
	}

	LOG_INF("Next connection retry in %d seconds",
		CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work,
			K_SECONDS(CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS));
}
void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		LOG_INF("AWS_IOT_EVT_CONNECTING");
		break;
	case AWS_IOT_EVT_CONNECTED:
		LOG_INF("AWS_IOT_EVT_CONNECTED");
		printk("AWS IOT event connected");

		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		 (void)k_work_cancel_delayable(&connect_work);

		if (evt->data.persistent_session) {
			LOG_INF("Persistent session enabled");
		}

#if defined(CONFIG_NRF_MODEM_LIB)
		/** Successfully connected to AWS IoT broker, mark image as
		 *  working to avoid reverting to the former image upon reboot.
		 */
		boot_write_img_confirmed();
#endif

		/** Send version number to AWS IoT broker to verify that the
		 *  FOTA update worked.
		 */
		// k_work_submit(&shadow_update_version_work);
 		//create_dummy_gnss(&last_pvt);
						   //print_fix_data(&last_pvt);			
		//shadow_update(&last_pvt);
		/** Start sequential shadow data updates.
		 */
		// k_work_schedule(&shadow_update_work,
		// 		K_SECONDS(CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS));
				//comment

#if defined(CONFIG_NRF_MODEM_LIB)
		int err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Requesting PSM failed, error: %d", err);
		}
#endif
		break;
	case AWS_IOT_EVT_READY:
		LOG_INF("AWS_IOT_EVT_READY");
		break;
	case AWS_IOT_EVT_DISCONNECTED:
		LOG_INF("AWS_IOT_EVT_DISCONNECTED");
		cloud_connected = false;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		// (void)k_work_cancel_delayable(&shadow_update_work);
		k_work_schedule(&connect_work, K_NO_WAIT);
		break;
	case AWS_IOT_EVT_DATA_RECEIVED:
		LOG_INF("AWS_IOT_EVT_DATA_RECEIVED");
		print_received_data(evt->data.msg.ptr, evt->data.msg.topic.str,
				    evt->data.msg.topic.len);
		break;
	case AWS_IOT_EVT_PUBACK:
		LOG_INF("AWS_IOT_EVT_PUBACK, message ID: %d", evt->data.message_id);
		break;
	case AWS_IOT_EVT_FOTA_START:
		LOG_INF("AWS_IOT_EVT_FOTA_START");
		break;
	case AWS_IOT_EVT_FOTA_ERASE_PENDING:
		LOG_INF("AWS_IOT_EVT_FOTA_ERASE_PENDING");
		LOG_INF("Disconnect LTE link or reboot");
#if defined(CONFIG_NRF_MODEM_LIB)
		err = lte_lc_offline();
		if (err) {
			LOG_ERR("Error disconnecting from LTE");
		}
#endif
		break;
	case AWS_IOT_EVT_FOTA_ERASE_DONE:
		LOG_INF("AWS_FOTA_EVT_ERASE_DONE");
		LOG_INF("Reconnecting the LTE link");
#if defined(CONFIG_NRF_MODEM_LIB)
		err = lte_lc_connect();
		if (err) {
			LOG_ERR("Error connecting to LTE");
		}
#endif
		break;
	case AWS_IOT_EVT_FOTA_DONE:
		LOG_INF("AWS_IOT_EVT_FOTA_DONE");
		LOG_INF("FOTA done, rebooting device");
		aws_iot_disconnect();
		sys_reboot(0);
		break;
	case AWS_IOT_EVT_FOTA_DL_PROGRESS:
		LOG_INF("AWS_IOT_EVT_FOTA_DL_PROGRESS, (%d%%)", evt->data.fota_progress);
	case AWS_IOT_EVT_ERROR:
		LOG_INF("AWS_IOT_EVT_ERROR, %d", evt->data.err);
		break;
	case AWS_IOT_EVT_FOTA_ERROR:
		LOG_INF("AWS_IOT_EVT_FOTA_ERROR");
		break;
	default:
		LOG_WRN("Unknown AWS IoT event type: %d", evt->type);
		break;
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");

		// k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_INF("%s", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

int post_counter = 60; // delay between posts to aws
int main(void)
{
	int err;
	cJSON_Init();
	InitUart();
	InitBleUart();
	InitI2CCharger();
	GetID3630I2C();

	LOG_INF("Starting GNSS AWS sample");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	}

	/* Initialize reference coordinates (if used). */
	if (sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) > 1 &&
	    sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) > 1) {
		ref_used = true;
		ref_latitude = atof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE);
		ref_longitude = atof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE);
	}

		err = aws_iot_init(NULL, aws_iot_event_handler);
	if (err) {
		LOG_ERR("AWS IoT library could not be initialized, error: %d", err);
	}

	/** Subscribe to customizable non-shadow specific topics
	 *  to AWS IoT backend.
	 */
	err = app_topics_subscribe();
	if (err) {
		LOG_ERR("Adding application specific topics failed, error: %d", err);
	}


	err = lte_lc_init_and_connect_async(lte_handler);/*lte lc initialization*/
	if (err) {
		LOG_ERR("Modem could not be configured, error: %d", err);
		return 0;
	}

	err = modem_info_init();/**/
	if (err) {
		LOG_ERR("Failed initializing modem info module, error: %d", err);
	}
	k_work_init_delayable(&connect_work, connect_work_fn);
	


	if (sample_init() != 0) {
		LOG_ERR("Failed to initialize sample");
		return -1;
	}

	if (gnss_init_and_start() != 0) {
		LOG_ERR("Failed to initialize and start GNSS");
		return -1;
	}

	k_work_schedule(&connect_work, K_NO_WAIT); /*Aws connect work shedule*/
	fix_timestamp = k_uptime_get();

	// err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);
	// if (err) {
	// 	LOG_ERR("Failed to deactivate LTE, error: %d", err);
	// 	return;
	// }

	while(1)
	{
		k_msleep(10);
	}

	return 0;
}

/**
 * @brief 	   : GNSS and LTE Handler Task
 * @param [in] : None
 * @param [out]: None
 * @return	   : None
*/
static void GpsTask()
{
	uint8_t cnt = 0;
	uint8_t count = 0;
	bool WifiStatus = false;
	bool BleStatus = false;
	long long llCurrentTick = 0;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;
	_sGnssConfig sGnssConfig = {0};
	printk("DEBUG: inside GPS TASK \r\n");

	for (;;) {
		//NRFX_DELAY_US(2000000);
		(void)k_poll(events, 2, K_FOREVER);

		if (events[0].state == K_POLL_STATE_SEM_AVAILABLE &&
		    k_sem_take(events[0].sem, K_NO_WAIT) == 0) {
			/* New PVT data available */

			if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)) {
				/* TTFF test mode. */

				/* Calculate the time GNSS has been blocked by LTE. */
				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
					time_blocked++;
				}
			} else if (IS_ENABLED(CONFIG_GNSS_SAMPLE_NMEA_ONLY)) {
				/* NMEA-only output mode. */

				if (output_paused()) {
					goto handle_nmea;
				}

				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
					print_distance_from_reference(&last_pvt);
				}
			} else {
				/* PVT and NMEA output mode. */

				if (output_paused()) {
					goto handle_nmea;
				}

				printf("\033[1;1H");
				printf("\033[2J");
				print_satellite_stats(&last_pvt);

				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
					printf("GNSS operation blocked by lte\n");
				}
				if (last_pvt.flags &
				    NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
					printf("Insufficient GNSS time windows\n");
				}
				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT) {
					printf("Sleep period(s) between PVT notifications\n");
				}
				// printf("-----------------------------------\n");
				// printk("satelite flag %d\n",last_pvt.flags);
				if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) 
				{
					printk("DEBUG: GNSS CONNECTED TRUE\r\n");
					gnss_connected = true;
					
					fix_timestamp = k_uptime_get();
					printf("\n Valid GNSS\n\n\n");

					// err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
					// if (err) {
					// LOG_ERR("Failed to activate LTE, error: %d", err);
					// return;
					// }
					// NRFX_DELAY_US(2000000);
					print_fix_data(&last_pvt);
					sGnssConfig.dLatitude = last_pvt.latitude;
					sGnssConfig.dLongitude = last_pvt.longitude;
					UpdateLocation(&sGnssConfig);
					SetLocationDataStatus(true);
					printk("DEBUG: Location successful\r\n");
					llCurrentTick = sys_clock_tick_get();
					WifiStatus    = GetWifiStatus();
					BleStatus     = GetBleStatus();
					printk("BLE flag : %d WIFI flag : %d \r\n", BleStatus , WifiStatus);
					if(!(WifiStatus || BleStatus ))
					{
						if ((llCurrentTick - llSysTick) >= (10 * 32768))
						{
							if(cloud_connected == true && gnss_connected == true)
							{
								printk("DEBUG: Connection successful\r\n");
								shadow_update(&last_pvt);
							}
							llSysTick = llCurrentTick;
						}
					}
					print_distance_from_reference(&last_pvt);
				}
				else{
					// printk("satelite flag %d\n",last_pvt.flags);
					gnss_connected = false;
					SetLocationDataStatus(false);
					//printf("Seconds since last fix: %d\n",
					//       (uint32_t)((k_uptime_get() - fix_timestamp) / 1000));
						    
						   //print_fix_data(&last_pvt);
						  count++;
					if(cloud_connected == true && (count > post_counter) )
					{
						printk("DEBUG: Dummy data send successful\r\n");
						create_dummy_gnss(&last_pvt);
						shadow_update(&last_pvt);
						count = 0;
					}
					else{
						//printk("\ncloud not connected\n");
					}
					cnt++;
					//printf("Searching [%c]\n", update_indicator[cnt%4]);
				}

				printf("\nNMEA strings:\n\n");
			}
		}

		//SendData("AT\n\r");

handle_nmea:
		if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE &&
		    k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0) {
			/* New NMEA data available */

			if (!output_paused()) {
				printf("%s", nmea_data->nmea_str);
			}
			k_free(nmea_data);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
		k_msleep(10);
	}
} 
static bool CheckForConfig()
{
	int nRetVal = 0;
	bool bRet = false;
    uint8_t uReadBuf[255] = {0};
    uint8_t uCredentialIdx = 0;

#ifdef NVS_ENABLE
    _sConfigData *psConfigData = NULL;

    psConfigData = GetConfigData();

	

	nRetVal = NvsRead(uReadBuf, (sizeof(_sConfigData) * 5 ), CONFIG_IDX); 

	if (nRetVal == sizeof(_sConfigData) * 5)
	{
		memcpy(psConfigData, (_sConfigData *)uReadBuf, sizeof(_sConfigData) * 5);

		printk("DEBUG : WIFI Timeout : %d\n", psConfigData[0].sConfigTimes.usWifiTimeout);
		printk("DEBUG : BLE Timeout : %d\n", psConfigData[0].sConfigTimes.usBleTimeout);
		printk("DEBUG : LTE Timeout : %d\n", psConfigData[0].sConfigTimes.usLteTimeout);
		uCredentialIdx = CheckLastConnectedStatus();
		sprintf(uReadBuf, "%s,%s", psConfigData[uCredentialIdx].sWifiCred.ucSsid, psConfigData[uCredentialIdx].sWifiCred.ucPwd);
		SetAPCredentials(uReadBuf);
		printk("DEBUG : Read string from flash %s!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", uReadBuf);
   
		bRet = true;
	
	}
   
#endif

    return bRet;
}

/**
 * @brief 	   : System Handler Task
 * @param [in] : None
 * @param [out]: None
 * @return	   : None
*/
static void SystemTask()
{

	int nRetVal = 0;
	uint16_t uPercent = 0;
	float fTemp = 0.00;
	int iPetmove;
	

	InitTimerTask();
#ifdef NVS_ENABLE
	nRetVal = NvsInit();

	if (nRetVal != 0)
	{
		
	}
	if (CheckForConfig())
	{
		printk("DEBUG : Config successfully");

	}
	else
	{
		printk("ERROR : Failed to configure");
	}
#endif
	while (1)
	{   

		ProcessWiFiMsgs();
		ProcessBleMsg();
		ProcessDeviceState();
    
    /* Only for testing */
		MC36XX_acc_t rawAccel = MC3630readRawAccel();
    // k_msleep(10); 
    printk("X axis: %d\n", rawAccel.XAxis);
    // printk("Y axis: %d\n", rawAccel.YAxis);
    // printk("Z axis: %d\n", rawAccel.ZAxis);
    
    /* Uncomment to test */
	//ReadI2CPMIC(&uPercent, &fTemp);
    if (fTemp && uPercent) 
		{
			printk("percentage Read from PMIC : %d%%, Temp Read from PMIC %f\n", uPercent, fTemp);
		}
	else 
		{
			printk("WARN : Failed");
		}

	k_msleep(10);

	}
}