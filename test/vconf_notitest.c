/*
 * vconf
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hakjoo ko <hakjoo.ko@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


#include <stdio.h>
#include <vconf.h>
#include <Ecore.h>
#include <gconf/gconf-client.h>

//#define CHECK_THREAD
//#define CHECK_AFTER_RUN

GMainLoop *event_loop;

void test_cb(keynode_t *key, void* data)
{
   printf("\x1b[105;37m[TEST]%s noti get\x1b[0m\n", vconf_keynode_get_name(key));
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
   //vconf_ignore_key_changed(vconf_keynode_get_name(key), test_cb);
//   sleep(1);
//   g_main_loop_quit(event_loop);
   return;
}


#ifdef CHECK_THREAD
#include <pthread.h>

void *test_thread1(void *arg)
{
   if(!vconf_notify_key_changed((char *)arg, test_cb, NULL))
      printf("%s added\n", (char *)arg);
   return NULL;
}
#endif

#ifdef CHECK_AFTER_RUN
gboolean timeout_handler(gpointer data)
{
   char *key = data;
   if(!vconf_notify_key_changed(key, test_cb, NULL))
      printf("%s added\n", key);
   return FALSE;
}
#endif

int main(int argc, char **argv)
{
   int i;
//   char temp[100]="/opt/var/kdb/";

   g_type_init();
   ecore_init();

   event_loop = g_main_loop_new(NULL, FALSE);

   for(i=1;i<argc;i++) {
      if(argv[i]) {
#ifdef CHECK_AFTER_RUN
         g_timeout_add_seconds(3, (GSourceFunc)timeout_handler, argv[i]);
#elif defined(CHECK_THREAD)
         pthread_t test_thread;

         pthread_create(&test_thread, NULL, test_thread1, argv[i]);
         pthread_detach(test_thread);
#else
         vconf_notify_key_changed(argv[i], test_cb, NULL);
#endif

//      strcat(temp, argv[1]);
//      open(temp, O_RDONLY);
      }
   }

//   g_main_loop_run(event_loop);
   ecore_main_loop_begin();

   return 0;
}
