/*
 * libslp-setting
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hakjoo Ko <hakjoo.ko@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <tet_api.h>
#include <vconf.h>
#include <string.h>

static char* KEY_DB = NULL;
static char* KEY_MEMORY = NULL;
static char* KEY_FILE = NULL;
static char* pBufferString = NULL;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_vconf_get_str_func_01(void);
static void utc_ApplicationFW_vconf_get_str_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_vconf_get_str_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_vconf_get_str_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	KEY_DB="db/tet/key_1";
	KEY_MEMORY="memory/tet/key_2";
	KEY_FILE="file/tet/key_3";
	pBufferString = "This is Test String";
	
	vconf_set_str(KEY_DB, pBufferString);
	vconf_set_str(KEY_MEMORY, pBufferString);
	vconf_set_str(KEY_FILE, pBufferString);
}

static void cleanup(void)
{
	vconf_unset(KEY_DB);
	vconf_unset(KEY_MEMORY);
	vconf_unset(KEY_FILE);
}

/**
 * @brief Positive test case of vconf_get_str()
 */
static void utc_ApplicationFW_vconf_get_str_func_01(void)
{
	char* pRetString = NULL;

   	pRetString = vconf_get_str(KEY_DB);
	if (pRetString == NULL || strncmp(pBufferString, pRetString, strlen(pRetString))) {
		tet_infoline("vconf_get_str() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	pRetString = vconf_get_str(KEY_MEMORY);
	if (pRetString == NULL || strncmp(pBufferString, pRetString, strlen(pRetString))) {
		tet_infoline("vconf_get_str() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	pRetString = vconf_get_str(KEY_FILE);
	if (pRetString == NULL || strncmp(pBufferString, pRetString, strlen(pRetString))) {
		tet_infoline("vconf_get_str() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init vconf_get_str()
 */
static void utc_ApplicationFW_vconf_get_str_func_02(void)
{
	char* pRetString = NULL;
	const char* InvalidBackend = "Invalid/tet/key_1";

   	pRetString = vconf_get_str(InvalidBackend);
	if (pRetString) {
		tet_infoline("vconf_get_str() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
