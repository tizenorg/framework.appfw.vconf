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

const char *vconfkeys[5][4]={
   {"db/timetest/key1", "db/timetest/key2", "db/timetest/key3", "db/timetest/key4"},
   {"file/timetest/key1", "file/timetest/key2", "file/timetest/key3", "file/timetest/key4"},
   {"memory/timetest/key1", "memory/timetest/key2", "memory/timetest/key3", "memory/timetest/key4"},
   {"gconf_l/timetest/key1", "gconf_l/timetest/key2", "gconf_l/timetest/key3", "gconf_l/timetest/key4"},
   {"gconf_d/timetest/key1", "gconf_d/timetest/key2", "gconf_d/timetest/key3", "gconf_d/timetest/key4"}
};

void test_cb(void* data)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);
   printf("\x1b[105;37m[VConf]\x1b[0m %d.%6d\n",(int)tv.tv_sec, (int)tv.tv_usec);
   printf("\x1b[105;37m[VConf](%s)noti get\x1b[0m\n", (char *)data);

   return;
}

void test_cb2(keynode_t *key, void* data)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);
   printf("\x1b[105;37m[VConf]\x1b[0m %d.%6d\n",(int)tv.tv_sec, (int)tv.tv_usec);
   printf("\x1b[105;37m[VConf]test_cb2 noti get\x1b[0m\n");

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
//   vconf_del_changed_cb2(vconf_keynode_get_name(key), test_cb2);
   return;
}

void set_operation(int i)
{
   keylist_t *kl=NULL;

   vconf_set_int(vconfkeys[i][0], 1);
   vconf_set_dbl(vconfkeys[i][1], 0.1);
   vconf_set_bool(vconfkeys[i][2], 1);
   vconf_set_str(vconfkeys[i][3], "this is time test");

   kl = vconf_keylist_new();
   vconf_keylist_add_dbl(kl, vconfkeys[i][0], 0.2);
   vconf_keylist_add_bool(kl, vconfkeys[i][1], 0);
   vconf_keylist_add_str(kl, vconfkeys[i][2], "this is time test");
   vconf_keylist_add_int(kl, vconfkeys[i][3], 2);
   vconf_set(kl);
   vconf_keylist_free(kl);
}


void get_operation(int i, const char* parent_dir)
{
   keylist_t *kl=NULL;
   keynode_t *temp_node;
   int get_int;
   double get_dbl;
   char *get_str;

   vconf_get_dbl(vconfkeys[i][0], &get_dbl);
   vconf_get_bool(vconfkeys[i][1], &get_int);
   get_str=vconf_get_str(vconfkeys[i][2]);
   free(get_str);
   vconf_get_int(vconfkeys[i][3], &get_int);

   kl = vconf_keylist_new();
   vconf_get(kl, parent_dir, 0);
   while((temp_node = vconf_keylist_nextnode(kl))) {
      switch(vconf_keynode_get_type(temp_node)) {
         case VCONF_TYPE_INT:
            printf("key = %s, value = %d\n",
                  vconf_keynode_get_name(temp_node), vconf_keynode_get_int(temp_node));
            break;
         case VCONF_TYPE_BOOL:
            printf("key = %s, value = %d\n",
                  vconf_keynode_get_name(temp_node), vconf_keynode_get_bool(temp_node));
            break;
         case VCONF_TYPE_DOUBLE:
            printf("key = %s, value = %f\n",
                  vconf_keynode_get_name(temp_node), vconf_keynode_get_dbl(temp_node));
            break;
         case VCONF_TYPE_STRING:
            printf("key = %s, value = %s\n",
                  vconf_keynode_get_name(temp_node), vconf_keynode_get_str(temp_node));
            break;
         default:
            printf("Unknown Type\n");
      }
   }
   vconf_keylist_free(kl);
}

void unset_operation(int i)
{
   vconf_unset(vconfkeys[i][0]);
   vconf_unset(vconfkeys[i][1]);
   vconf_unset(vconfkeys[i][2]);
   vconf_unset(vconfkeys[i][3]);
}



int main(int argc, char **argv)
{
   int operation;

   g_type_init();
   ecore_init();
   if(argc < 2) {
      fprintf(stderr, "1:set(db), 2:set(file), 3:set(memory), 4:set(gconf_d), 5:set(gconf_l) \n");
      fprintf(stderr, "6:get(db), 7:get(file), 8:get(memory), 9:get(gconf_d), 10:get(gconf_l) \n");
      fprintf(stderr, "11:unset(db), 12:unset(file), 13:unset(memory), 14:unset(gconf_d), 15:unset(gconf_l) \n");
      return -1;
   }

   operation = atoi(argv[1]);
   switch(operation)
   {
      case 1:
         set_operation(0);
         break;
      case 2:
         set_operation(1);
         break;
      case 3:
         set_operation(2);
         break;
      case 4:
         set_operation(3);
         break;
      case 5:
         set_operation(4);
         break;
      case 6:
         get_operation(0, "db/timetest");
         break;
      case 7:
         get_operation(1, "file/timetest");
         break;
      case 8:
         get_operation(2, "memory/timetest");
         break;
      case 9:
         get_operation(3, "gconf_l/timetest");
         break;
      case 10:
         get_operation(4, "gconf_d/timetest");
         break;
      case 11:
         unset_operation(0);
         break;
      case 12:
         unset_operation(1);
         break;
      case 13:
         unset_operation(2);
         break;
      case 14:
         unset_operation(3);
         break;
      case 15:
         unset_operation(4);
         break;
      case 16:
         vconf_notify_key_changed(vconfkeys[0][1], test_cb2, NULL);
         vconf_notify_key_changed(vconfkeys[1][1], test_cb2, NULL);
         vconf_notify_key_changed(vconfkeys[2][1], test_cb2, NULL);
         vconf_notify_key_changed(vconfkeys[3][1], test_cb2, NULL);
         vconf_notify_key_changed(vconfkeys[4][1], test_cb2, NULL);

         //ecore_main_loop_begin();
         GMainLoop *event_loop;
         event_loop = g_main_loop_new(NULL, FALSE);
         g_main_loop_run(event_loop);
         break;
   }

   return 0;
}
