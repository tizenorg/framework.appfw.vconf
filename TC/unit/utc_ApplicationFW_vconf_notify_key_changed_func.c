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
#include <stdio.h>
#include <vconf.h>
#include <glib.h>
#include <glib-object.h>
#include <pthread.h>

static GMainLoop* event_loop = NULL;
static char* KEY_DB = NULL;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_vconf_notify_key_changed_func_01(void);
static void utc_ApplicationFW_vconf_notify_key_changed_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_vconf_notify_key_changed_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_vconf_notify_key_changed_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

void *test_thread1(void *arg)
{
	vconf_set_int(KEY_DB, 100);
	return NULL;
}

void test_cb(keynode_t* pKey, void* pData)
{	
	switch(vconf_keynode_get_type(pKey))
	{
	   case VCONF_TYPE_INT:
	      printf("key = %s, value = %d(int)\n", vconf_keynode_get_name(pKey), vconf_keynode_get_int(pKey));
	      break;
	   default:
	      break;
	}

	tet_result(TET_PASS);
	g_main_loop_quit(event_loop);
}

static void startup(void)
{
	KEY_DB="db/tet/key_1";
	vconf_set_int(KEY_DB, 10);

	event_loop = g_main_loop_new(NULL, FALSE);
}

static void cleanup(void)
{
	vconf_ignore_key_changed(KEY_DB, test_cb);
	vconf_unset(KEY_DB);
}

/**
 * @brief Positive test case of vconf_notify_key_changed()
 */
static void utc_ApplicationFW_vconf_notify_key_changed_func_01(void)
{
	pthread_t test_thread;
	int r = 0;

   	r = vconf_notify_key_changed(KEY_DB, test_cb, NULL);
	if (r) {
		tet_infoline("vconf_notify_key_changed() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	pthread_create(&test_thread, NULL, test_thread1, NULL);
	pthread_detach(test_thread);

	g_main_loop_run(event_loop);
}

/**
 * @brief Negative test case of ug_init vconf_notify_key_changed()
 */
static void utc_ApplicationFW_vconf_notify_key_changed_func_02(void)
{
	int r = 0;

   	r = vconf_notify_key_changed(KEY_DB, NULL, NULL);
	if (!r) {
		tet_infoline("vconf_notify_key_changed() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
