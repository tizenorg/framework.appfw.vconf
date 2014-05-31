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

keylist_t* pKeyList = NULL;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_vconf_keylist_new_func_01(void);
static void utc_ApplicationFW_vconf_keylist_new_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_vconf_keylist_new_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_vconf_keylist_new_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	if(pKeyList!=NULL)
	{
		vconf_keylist_free(pKeyList);
		pKeyList = NULL;
	}
}

static void cleanup(void)
{
	if(pKeyList!=NULL)
	{
		vconf_keylist_free(pKeyList);
		pKeyList = NULL;
	}
}

/**
 * @brief Positive test case of vconf_keylist_new()
 */
static void utc_ApplicationFW_vconf_keylist_new_func_01(void)
{
	pKeyList = vconf_keylist_new();	
	if (!pKeyList) {
		tet_infoline("vconf_keylist_new() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init vconf_keylist_new()
 */
static void utc_ApplicationFW_vconf_keylist_new_func_02(void)
{
	tet_result(TET_PASS);
}
