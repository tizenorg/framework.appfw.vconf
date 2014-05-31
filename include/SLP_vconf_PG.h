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

/**
 *  @ingroup StorageFW
 *  @defgroup StorageFW_VCONF_MODULE VCONF
 *  @{

 <h1 class="pg">Introduction</h1>
	<p> VCONF is a virtual configuration system which provides three alternative back-ends for text configuration files and notification.
		It internally uses a modified Elektra 0.7 and Inotifiy system for implementation. </p>

<h1 class="pg">Features</h1>
	<ul>
		<li> It provides the key value pair mechanism for system configuration data. </li>
		<li> It supports multiple data types (bool, int, double and string) for configuration data.</li>
		<li>It supports key value changed notification.</li>
		<li> <ol> It provides three type of backends
			<li> DB</li>
			<li> FILe</li>
			<li> Memory</li>
		</ol></li>
		<li>It provides the option to select the backend to store the configuration data.</li>

	</ul>

<h1 class="pg">VCONF Logical View diagram</h1>
\image html SLP_VCONF_PG_images_logical_view.png "Picture 1. Logical view"

<h1 class="pg"> vconf functional diagram </h1>
 \image html SLP_Vconf_PG_image01.png "Picture 2.Functional view"

 <h2 class="pg">Properties</h2>
	<ul>
		<li> Convenient API (read/write common data type : integer, double, boolean, string) </li>
		<li> Guarantee Transaction(db backend) </li>
		<li> Apply Key-List concept </li>
		<li> Changeable Backend (db, file, memory) </li>
		<li> Simple Notification based Inotify </li>
	</ul>

<h1 class="pg">Programming Guide</h1>

<h2 class="pg"> Header file </h1>
<strong>header file name : vconf.h </strong>

<h2 class="pg">Backends</h1>
	<p> VConf has 3 kinds of backend.</p>
	<ol>
		<li>db backend</li>
			<p> We can use this backend by specifying the "db" prefix. E.g. db/test_app/key1 </p></br>

			<p> A key starting with db is saved at a location which is mounted by libsqlfs.
				Therefore the key has characteristics of a database.(robustness, atomicity).
				But it is slower than the other backends. </p><br/>

			<p> Developers can use this backend in order to synchronize immediately. </p><br/>

@code
#include <stdio.h>
#include <vconf.h>

const char *key1_name="db/test/key1";

int main(int argc, char **argv)
{
   int key1_value;

   if(vconf_set_int(key1_name,1))
      fprintf(stderr, "vconf_set_int FAIL\n");
   else
      printf("vconf_set_int OK\n");

   if(vconf_get_int(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_int FAIL\n");
   else
      printf("vconf_get_int OK(key1 value is %d)\n", key1_value);

   return 0;
}
@endcode

	<li> memory backend</li>
	<p> We can use this backend by specifying the "memory" prefix. E.g. memory/test_app/key1</p> <br>

	<p> A key starting with memory is saved at a location which is mounted by tmpfs.
		Therefore the key has characteristics of memory. It has the highest speed.
		But if the target is turned off, the key is removed (volatile). </p><br>

	<p> Developers can use this backend for keys such as the power state of phone.</p> <br/>

@code
#include <stdio.h>
#include <vconf.h>

 const char *key1_name="memory/test/key1";

 int main(int argc, char **argv)
 {
   int key1_value;

   if(vconf_set_bool(key1_name, 1))
      fprintf(stderr, "vconf_set_bool FAIL\n");
   else
      printf("vconf_set_bool OK\n");

   if(vconf_get_bool(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_bool FAIL\n");
   else
      printf("vconf_get_bool OK(key1 value is %d)\n", key1_value);

   return 0;
 }
@endcode

	<li>File backend</li>
		<p>	We can use this backend by specifying the "file" prefix.E.g. file/test_app/key1 </p><br/>

		<p>	A key starting with file is saved at a location which is mounted by basic file system.
		Therefore the key has characteristics of file. If the target is turned off without synchronising, the key can lose data. </p><br/>

		<p>	Developers can use this backend for keys such as the sync state of something,
		where it is not vitally important for the key to hold the correct data. </p><br/>

@code
#include <stdio.h>
#include <vconf.h>

const char *key1_name="file/test/key1";

int main(int argc, char **argv)
{
   double key1_value;

   if(vconf_set_dbl(key1_name, 1.23))
      fprintf(stderr, "vconf_set_dbl FAIL\n");
   else {
      printf("vconf_set_dbl OK\n");
      vconf_sync_key(key1_name);
   }

   if(vconf_get_dbl(key1_name, &key1_value))
      fprintf(stderr, "vconf_get_dbl FAIL\n");
   else
      printf("vconf_get_dbl OK(key1 value is %f)\n", key1_value);

   return 0;
}
@endcode

<h2 class="pg">Notification</h1>
	<p>	The notification of a changed key is handled in the g_main_loop of the default context.
	If you want to receive a notification, you should use g_main_loop of the default context.
	For g_main_loop, refer to glib manual. </p><br/>

@code
#include <stdio.h>
#include <vconf.h>
#include <glib-object.h>
#include <glib.h>

const char *key1_name="file/test/key1";

void test_cb(keynode_t *key, void* data)
{
   switch(vconf_keynode_get_type(key))
   {
      case VCONF_TYPE_INT:
	printf("key = %s, value = %d(int)\n",
		vconf_keynode_get_name(key), vconf_keynode_get_int(key));
	break;
      case VCONF_TYPE_BOOL:
	printf("key = %s, value = %d(bool)\n",
		vconf_keynode_get_name(key), vconf_keynode_get_bool(key));
	break;
      case VCONF_TYPE_DOUBLE:
	printf("key = %s, value = %f(double)\n",
		vconf_keynode_get_name(key), vconf_keynode_get_dbl(key));
	break;
      case VCONF_TYPE_STRING:
	printf("key = %s, value = %s(string)\n",
		vconf_keynode_get_name(key), vconf_keynode_get_str(key));
	break;
      default:
	fprintf(stderr, "Unknown Type(%d)\n", vconf_keynode_get_type(key));
	break;
   }
   return;
}

int main()
{
   GMainLoop *event_loop;

   vconf_notify_key_changed(key1_name, test_cb, NULL);

   event_loop = g_main_loop_new(NULL, FALSE);
   g_main_loop_run(event_loop);

   vconf_ignore_key_changed(key1_name, test_cb);

   return 0;
}
@endcode
*/

/**
@}
*/
