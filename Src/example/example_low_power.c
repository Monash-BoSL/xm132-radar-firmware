// Copyright (c) Acconeer AB, 2019-2021
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "acc_definitions.h"
#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_envelope.h"
#include "acc_service_sparse.h"
#include "acc_version.h"

/**
 * @brief Example that shows a single update (using envelope service execute once) and then enters low power mode
 *
 */

#ifndef SUSPEND_TIME_BETWEEN_UPDATES_MS
#define SUSPEND_TIME_BETWEEN_UPDATES_MS (10000) // 0.1Hz
#endif

#if !defined(USE_SERVICE_ENVELOPE) && !defined(USE_SERVICE_SPARSE)
#define USE_SERVICE_ENVELOPE
#endif

#ifndef POWER_SAVE_MODE
#define POWER_SAVE_MODE OFF
#endif

#ifndef RANGE_LENGTH
#define RANGE_LENGTH (0.6f)
#endif

#ifndef SERVICE_PROFILE
#define SERVICE_PROFILE 2
#endif

#define PASTER(x, y)    x ## y
#define EVALUATOR(x, y) PASTER(x, y)

#define SELECTED_POWER_SAVE_MODE (EVALUATOR(ACC_POWER_SAVE_MODE_, POWER_SAVE_MODE))
#define SELECTED_SERVICE_PROILE  (EVALUATOR(ACC_SERVICE_PROFILE_, SERVICE_PROFILE))


/**
 * @brief ID of service to use
 */
typedef enum
{
	SERVICE_ID_ENVELOPE,
	SERVICE_ID_SPARSE,
	SERVICE_ID_UNKNOWN,
} service_id_t;


/**
 * @brief Function for getting current service ID
 */
static service_id_t get_service_type(void)
{
#if defined(USE_SERVICE_ENVELOPE)
	return SERVICE_ID_ENVELOPE;
#elif defined(USE_SERVICE_SPARSE)
	return SERVICE_ID_SPARSE;
#else
	return SERVICE_ID_UNKNOWN;
#endif
}


/**
 * @brief Function for creating envelope service
 */
static acc_service_handle_t create_envelope_service(void)
{
	bool                 success        = true;
	acc_service_handle_t service_handle = NULL;

	float range_start_m  = 1.4f;
	float range_length_m = RANGE_LENGTH;

	acc_service_configuration_t envelope_configuration = acc_service_envelope_configuration_create();

	if (envelope_configuration == NULL)
	{
		printf("Could not create envelope configuration\n");
		success = false;
	}

	if (success)
	{
		acc_service_requested_start_set(envelope_configuration, range_start_m);
		acc_service_requested_length_set(envelope_configuration, range_length_m);
		acc_service_power_save_mode_set(envelope_configuration, SELECTED_POWER_SAVE_MODE);
		acc_service_profile_set(envelope_configuration, SELECTED_SERVICE_PROILE);

		service_handle = acc_service_create(envelope_configuration);

		if (service_handle == NULL)
		{
			printf("Could not create envelope service\n");
			success = false;
		}
	}

	acc_service_envelope_configuration_destroy(&envelope_configuration);

	return success ? service_handle : NULL;
}


/**
 * @brief Function for creating sparse service
 */
static acc_service_handle_t create_sparse_service(void)
{
	bool                 success        = true;
	acc_service_handle_t service_handle = NULL;

	float range_start_m  = 1.4f;
	float range_length_m = RANGE_LENGTH;

	acc_service_configuration_t sparse_configuration = acc_service_sparse_configuration_create();

	if (sparse_configuration == NULL)
	{
		printf("Could not create sparse configuration\n");
		success = false;
	}

	if (success)
	{
		acc_service_requested_start_set(sparse_configuration, range_start_m);
		acc_service_requested_length_set(sparse_configuration, range_length_m);
		acc_service_power_save_mode_set(sparse_configuration, SELECTED_POWER_SAVE_MODE);
		acc_service_profile_set(sparse_configuration, SELECTED_SERVICE_PROILE);

		service_handle = acc_service_create(sparse_configuration);

		if (service_handle == NULL)
		{
			printf("Could not create sparse service\n");
			success = false;
		}
	}

	acc_service_sparse_configuration_destroy(&sparse_configuration);

	return success ? service_handle : NULL;
}


/**
 * @brief Function for creating service
 */
static acc_service_handle_t create_service(void)
{
	switch (get_service_type())
	{
		case SERVICE_ID_ENVELOPE:
			return create_envelope_service();

		case SERVICE_ID_SPARSE:
			return create_sparse_service();

		default:
			return NULL;
	}
}


/**
 * @brief Function for running the envelope service
 */
static void execute_envelope_service(acc_service_handle_t handle, uint16_t *result1, uint16_t *result2)
{
	acc_service_envelope_metadata_t metadata = { 0 };

	acc_service_envelope_get_metadata(handle, &metadata);

	uint16_t                           envelope_data[metadata.data_length];
	acc_service_envelope_result_info_t result_info;

	uint16_t max_amplitude_index = 0;
	uint16_t max_amplitude       = 0;

	if (acc_service_envelope_get_next(handle, envelope_data, metadata.data_length, &result_info))
	{
		max_amplitude_index = 0;
		max_amplitude       = 0;

		for (uint16_t index = 0; index < metadata.data_length; index++)
		{
			if (envelope_data[index] > max_amplitude)
			{
				max_amplitude       = envelope_data[index];
				max_amplitude_index = index;
			}
		}
	}

	*result1 = max_amplitude;
	*result2 = max_amplitude_index;
}


/**
 * @brief Function for running the sparse service
 */
static void execute_sparse_service(acc_service_handle_t handle, uint16_t *result1, uint16_t *result2)
{
	acc_service_sparse_metadata_t metadata = { 0 };

	acc_service_sparse_get_metadata(handle, &metadata);

	uint16_t                         sparse_data[metadata.data_length];
	acc_service_sparse_result_info_t result_info;

	if (acc_service_sparse_get_next(handle, sparse_data, metadata.data_length, &result_info))
	{
		*result1 = sparse_data[0];
		*result2 = metadata.data_length;
	}
}


/**
 * @brief Function for running a service
 */
static void execute_service(acc_service_handle_t handle, uint16_t *result1, uint16_t *result2)
{
	switch (get_service_type())
	{
		case SERVICE_ID_ENVELOPE:
			execute_envelope_service(handle, result1, result2);
			break;

		case SERVICE_ID_SPARSE:
			execute_sparse_service(handle, result1, result2);
			break;

		default:
			*result1 = 0;
			*result2 = 0;
			break;
	}
}


bool acconeer_example(void);


bool acconeer_example(void)
{
	printf("Acconeer software version %s\n", acc_version_get());

	acc_hal_t hal = *acc_hal_integration_get_implementation();

	acc_integration_set_periodic_wakeup(SUSPEND_TIME_BETWEEN_UPDATES_MS);

	hal.log.log_level = ACC_LOG_LEVEL_ERROR;

	if (!acc_rss_activate(&hal))
	{
		return false;
	}

	acc_service_handle_t handle = create_service();
	if (handle == NULL)
	{
		printf("acc_service_create() failed\n");
		return false;
	}

	if (!acc_service_activate(handle))
	{
		printf("acc_service_activate() failed\n");
		acc_service_destroy(&handle);
		acc_rss_deactivate();
		return false;
	}

	while (true)
	{
		uint16_t result1 = 0;
		uint16_t result2 = 0;

		execute_service(handle, &result1, &result2);

		printf("Service data %" PRIu16 " , %" PRIu16 "\n", result1, result2);

		acc_integration_sleep_until_periodic_wakeup();
	}

	acc_service_deactivate(handle);

	acc_service_destroy(&handle);

	acc_rss_deactivate();

	return true;
}
