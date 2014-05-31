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
static char* KEY_PARENT = NULL;
static char* KEY_01 = NULL;
static char* KEY_02 = NULL;
static char* KEY_03 = NULL;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_vconf_keylist_nextnode_func_01(void);
static void utc_ApplicationFW_vconf_keylist_nextnode_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_vconf_keylist_nextnode_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_vconf_keylist_nextnode_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	KEY_PARENT = "db/tet";
	KEY_01="db/tet/key_1";
	KEY_02="db/tet/key_2";
	KEY_03="db/tet/key_3";

	if(pKeyList!=NULL)
	{
		vconf_keylist_free(pKeyList);
		pKeyList = NULL;
	}

	pKeyList = vconf_keylist_new();

	vconf_keylist_add_bool(pKeyList, KEY_01, 0);
	vconf_keylist_add_int(pKeyList, KEY_02, 100);
	vconf_keylist_add_dbl(pKeyList, KEY_03, 0.453);

	vconf_keylist_free(pKeyList);
	pKeyList = NULL;
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
 * @brief Positive test case of vconf_keylist_nextnode()
 */
static void utc_ApplicationFW_vconf_keylist_nextnode_func_01(void)
{
	int r;

	pKeyList = vconf_keylist_new();
	
	r = vconf_get(pKeyList, KEY_PARENT, VCONF_GET_KEY);	
	if (r) {
		tet_infoline("vconf_get() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	while(vconf_keylist_nextnode(pKeyList)) ;

	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init vconf_keylist_nextnode()
 */
static void utc_ApplicationFW_vconf_keylist_nextnode_func_02(void)
{
	// There is no Negative test case for vconf_keylist_nextnode function.
	
	tet_result(TET_PASS);
}
