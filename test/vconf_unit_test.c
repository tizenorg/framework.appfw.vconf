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
#include <dirent.h>

const char *vconfkeys[5][5]={
   {"db", "db/unittest/key1", "db/unittest/key2", "db/unittest/key3", "db/unittest/key4"},
   {"file", "file/unittest/key1", "file/unittest/key2", "file/unittest/key3", "file/unittest/key4"},
   {"memory", "memory/unittest/key1", "memory/unittest/key2", "memory/unittest/key3", "memory/unittest/key4"},
   {"gconf_l", "gconf_l/unittest/key1", "gconf_l/unittest/key2", "gconf_l/unittest/key3", "gconf_l/unittest/key4"},
   {"gconf_d", "gconf_d/unittest/key1", "gconf_d/unittest/key2", "gconf_d/unittest/key3", "gconf_d/unittest/key4"}
};

#define EPRINT(FMT, ARG...) \
   fprintf(stderr, "\x1b[31m[VConf Unit Test]\x1b[0m" FMT "\n", ##ARG)

#define PRINT(FMT, ARG...) \
   fprintf(stdout, "\x1b[33m[VConf Unit Test]\x1b[0m" FMT "\n", ##ARG)

static GMainLoop *event_loop;

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
      case VCONF_TYPE_NONE:
         printf("key = %s, deleted\n", vconf_keynode_get_name(key));
         if(data == (void *)vconfkeys[4][0]) g_main_loop_quit(event_loop);
         break;
      default:
         fprintf(stderr, "Unknown Type(%d)\n", vconf_keynode_get_type(key));
         break;
   }

   PRINT("%s Notification OK", (char *)data);
   return;
}

void set_operation(int i)
{
   keylist_t *kl=NULL;

   if(vconf_set_int(vconfkeys[i][1], 1))
      EPRINT("vconf_set_int FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_set_int OK(%s)", vconfkeys[i][0]);
      
   if(vconf_set_dbl(vconfkeys[i][2], 0.1))
      EPRINT("vconf_set_dbl FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_set_dbl OK(%s)", vconfkeys[i][0]);

   if(vconf_set_bool(vconfkeys[i][3], 1))
      EPRINT("vconf_set_bool FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_set_bool OK(%s)", vconfkeys[i][0]);

   if(vconf_set_str(vconfkeys[i][4], "This is UNIT test"))
      EPRINT("vconf_set_str FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_set_str OK(%s)", vconfkeys[i][0]);

   kl = vconf_keylist_new();
   vconf_keylist_add_dbl(kl, vconfkeys[i][1], 0.2);
   vconf_keylist_add_bool(kl, vconfkeys[i][2], 0);
   vconf_keylist_add_str(kl, vconfkeys[i][3], "This is UNIT test");
   vconf_keylist_add_int(kl, vconfkeys[i][4], 2);
   if(vconf_set(kl))
      EPRINT("vconf_set FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_set OK(%s)", vconfkeys[i][0]);
   vconf_keylist_free(kl);

}


void get_operation(int i, const char* parent_dir)
{
   keylist_t *kl=NULL;
   keynode_t *temp_node;
   int get_int;
   double get_dbl;
   char *get_str;

   if(vconf_get_dbl(vconfkeys[i][1], &get_dbl) || get_dbl != 0.2)
      EPRINT("vconf_get_dbl FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_get_dbl OK(%s)", vconfkeys[i][0]);

   if(vconf_get_bool(vconfkeys[i][2], &get_int) || get_int != 0)
      EPRINT("vconf_get_bool FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_get_bool OK(%s)", vconfkeys[i][0]);

   get_str=vconf_get_str(vconfkeys[i][3]);
   if(get_str && !strcmp(get_str, "This is UNIT test"))
      PRINT("vconf_get_str OK(%s)", vconfkeys[i][0]);
   else
      EPRINT("vconf_get_str FAIL(%s)", vconfkeys[i][0]);
   if(get_str) free(get_str);

   if(vconf_get_int(vconfkeys[i][4], &get_int) || get_int != 2)
      EPRINT("vconf_get_int FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_get_int OK(%s)", vconfkeys[i][0]);

   kl = vconf_keylist_new();
   if(vconf_get(kl, parent_dir, 0))
      EPRINT("vconf_get FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_get OK(%s)", vconfkeys[i][0]);

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
   if(vconf_unset(vconfkeys[i][1]))
      EPRINT("vconf_unset FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_unset OK(%s)", vconfkeys[i][0]);
   if(vconf_unset(vconfkeys[i][2]))
      EPRINT("vconf_unset FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_unset OK(%s)", vconfkeys[i][0]);
   if(vconf_unset(vconfkeys[i][3]))
      EPRINT("vconf_unset FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_unset OK(%s)", vconfkeys[i][0]);
   if(vconf_unset(vconfkeys[i][4]))
      EPRINT("vconf_unset FAIL(%s)", vconfkeys[i][0]);
   else
      PRINT("vconf_unset OK(%s)", vconfkeys[i][0]);
}

const char *test_dir[5]={
   "/opt/var/kdb/db/unittest", "/opt/var/kdb/file/unittest", "/var/run/memory/unittest",
   "/opt/var/gconf/local/unittest", "/opt/var/gconf/unittest"};

void recursive_unset_operation(void)
{
   char parent[1024];
   int i,j;
   for(i=0;i<5;i++) {
      for(j=1;j<5;j++)
         vconf_set_int(vconfkeys[i][j],1);

      sprintf(parent, "%s/unittest", vconfkeys[i][0]);
      if(vconf_unset_recursive(parent))
         EPRINT("vconf_unset_recursive FAIL(%s)", parent);
      else {
         if(opendir(test_dir[i]))
            EPRINT("vconf_unset_recursive FAIL(%s)", parent);
         else
            PRINT("vconf_unset_recursive OK(%s)", parent);
      }
   }
}

void notify_operation(int i)
{
   int j;
   for(j=1;j<5;j++)
   {
      if(vconf_notify_key_changed(vconfkeys[i][j], test_cb, (void *)vconfkeys[i][0])) {
         if(3==i)
            PRINT("vconf_notify_key_changed OK(%s)", vconfkeys[i][j]);
         else
            EPRINT("vconf_notify_key_changed FAIL(%s)", vconfkeys[i][j]);
      }else {
         if(3==i)
            EPRINT("vconf_notify_key_changed FAIL(%s)", vconfkeys[i][j]);
         else               
            PRINT("vconf_notify_key_changed OK(%s)", vconfkeys[i][j]);
      }
   }
}
   
int main(int argc, char **argv)
{
   g_type_init();

   set_operation(0);
   set_operation(1);
   set_operation(2);
   set_operation(3);
   set_operation(4);
   get_operation(0, "db/unittest");
   get_operation(1, "file/unittest");
   get_operation(2, "memory/unittest");
   get_operation(3, "gconf_l/unittest");
   get_operation(4, "gconf_d/unittest");
   unset_operation(0);
   unset_operation(1);
   unset_operation(2);
   unset_operation(3);
   unset_operation(4);
   recursive_unset_operation();
   notify_operation(0);
   notify_operation(1);
   notify_operation(2);
   notify_operation(3);
   notify_operation(4);
   printf("For Notification TEST, You have to excute below command\n");
   printf("vconftool set -t int db/unittest/key1 1\n");
   printf("vconftool set -t int file/unittest/key2 1\n");
   printf("vconftool set -t int memory/unittest/key3 1\n");
   printf("vconftool set -t int gconf_d/unittest/key4 1\n");
   printf("vconftool unset db/unittest/key1\n");
   printf("vconftool unset file/unittest/key2\n");
   printf("vconftool unset memory/unittest/key3\n");
   printf("vconftool unset gconf_d/unittest/key4\n");

   //ecore_main_loop_begin();
   event_loop = g_main_loop_new(NULL, FALSE);
   g_main_loop_run(event_loop);

   EPRINT("Did you get 8 Notifications\n");

   return 0;
}
