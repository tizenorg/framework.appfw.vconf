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
#include <stdio.h>
#include <string.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_vconf_unset_recursive_func_01(void);
static void utc_ApplicationFW_vconf_unset_recursive_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_vconf_unset_recursive_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_vconf_unset_recursive_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{	
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of vconf_unset_recursive()
 */
static void utc_ApplicationFW_vconf_unset_recursive_func_01(void)
{
	int r = 0;
	char* pszBackend[3] = {"db", "memory", "file"};
	char szBuffer[1024] = {0, };
	int i, j;

	for(i=0; i<3; ++i)
	{
		for(j=0; j<5; ++j)
		{
			snprintf(szBuffer, 1024, "%s/tet/key_%d", pszBackend[i], j);
			vconf_set_int(szBuffer, 10);
			memset(szBuffer, 0x00, sizeof(szBuffer));
		}
	}

	for(i=0; i<3; ++i)
	{
		snprintf(szBuffer, 1024, "%s/tet", pszBackend[i]);
		r = vconf_unset_recursive(szBuffer);
		if (r) {
			tet_infoline("vconf_unset_recursive() failed in positive test case");
			tet_result(TET_FAIL);
			return;
		}
		memset(szBuffer, 0x00, sizeof(szBuffer));
	}
   	
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init vconf_unset_recursive()
 */
static void utc_ApplicationFW_vconf_unset_recursive_func_02(void)
{
	int r = 0;
	const char* InvalidBackend = "Invalid/tet";

   	r = vconf_unset_recursive(InvalidBackend);
	if (!r) {
		tet_infoline("vconf_unset_recursive() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
