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

#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <linux/version.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include "vconf-internals.h"

#include <glib.h>

#define INOTY_EVENT_MASK   (IN_CLOSE_WRITE | IN_DELETE_SELF)

/* inotify */
struct noti_node {
	int wd;
	char *keyname;
	vconf_callback_fn cb;
	void *cb_data;
	struct noti_node *next;
};
typedef struct noti_node noti_node_s;
static GList *g_notilist;

static int _vconf_inoti_comp_with_wd(gconstpointer a, gconstpointer b)
{
	int r;

	noti_node_s *key1 = (noti_node_s *) a;
	noti_node_s *key2 = (noti_node_s *) b;

	r = key1->wd - key2->wd;
	return r;
}

static int _vconf_inoti_comp_with_wd_cb(gconstpointer a, gconstpointer b)
{
	int r;

	noti_node_s *key1 = (noti_node_s *) a;
	noti_node_s *key2 = (noti_node_s *) b;

	r = key1->wd - key2->wd;
	if (r != 0)
		return r;

	r = (int)(key1->cb - key2->cb);
	return r;
}

static int _kdb_inoti_fd;

static pthread_mutex_t _kdb_inoti_fd_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _kdb_g_ns_mutex = PTHREAD_MUTEX_INITIALIZER;

static GSource *_kdb_handler;

static GList* _vconf_copy_noti_list(GList *orig_notilist)
{
	GList *copy_notilist = NULL;
	struct noti_node *n = NULL;
	struct noti_node *t = NULL;

	if (!orig_notilist)
		return NULL;

	orig_notilist = g_list_first(orig_notilist);
	if (!orig_notilist)
		return NULL;

	while(orig_notilist) {
		t = orig_notilist->data;

		n = calloc(1, sizeof(noti_node_s));
		if (n == NULL) {
			ERR("_vconf_copy_noti_list : calloc failed. memory full");
			break;
		}

		n->wd = t->wd;
		n->keyname = strndup(t->keyname, VCONF_KEY_PATH_LEN);
		n->cb_data = t->cb_data;
		n->cb = t->cb;

		copy_notilist = g_list_append(copy_notilist, n);
		orig_notilist = g_list_next(orig_notilist);
	}
	return copy_notilist;

}

static void _vconf_free_noti_node(gpointer data)
{
	struct noti_node *n = (struct noti_node*)data;
	g_free(n->keyname);
	g_free(n);
}

static void _vconf_free_noti_list(GList *noti_list)
{
	g_list_free_full(noti_list, _vconf_free_noti_node);
}


static gboolean _vconf_kdb_gio_cb(GIOChannel *src, GIOCondition cond, gpointer data)
{
	int fd, r;
	struct inotify_event ie;
	GList *l_notilist = NULL;

	//INFO("vconf noti function");

	fd = g_io_channel_unix_get_fd(src);
	r = read(fd, &ie, sizeof(ie));

	while (r > 0) {
		//INFO("read event from GIOChannel. pid : %d", getpid());

		if (ie.mask & INOTY_EVENT_MASK) {

			pthread_mutex_lock(&_kdb_g_ns_mutex);
			l_notilist = _vconf_copy_noti_list(g_notilist);
			pthread_mutex_unlock(&_kdb_g_ns_mutex);

			if (l_notilist) {

				struct noti_node *t = NULL;
				GList *noti_list = NULL;

				noti_list = g_list_first(l_notilist);

				while (noti_list) {
					t = noti_list->data;

					keynode_t* keynode = _vconf_keynode_new();
					if (keynode == NULL) {
						ERR("key malloc fail");
						break;
					}

					if( (t) && (t->wd == ie.wd) ) {
						if ((ie.mask & IN_DELETE_SELF)) {
							INFO("Notify that key(%s) is deleted", t->keyname);
							_vconf_keynode_set_keyname(keynode, (const char *)t->keyname);
							_vconf_keynode_set_null(keynode);
							t->cb(keynode, t->cb_data);
							_vconf_kdb_del_notify(t->keyname, t->cb);
						} else {
							_vconf_keynode_set_keyname(keynode, t->keyname);
							_vconf_get_key(keynode);
							INFO("key(%s) is changed. cb(%p) called", t->keyname, t->cb);
							t->cb(keynode, t->cb_data);
						}
					}

					_vconf_keynode_free(keynode);

					noti_list = g_list_next(noti_list);
				}

				_vconf_free_noti_list(l_notilist);
			}
		}

		if (ie.len > 0)
			(void) lseek(fd, ie.len, SEEK_CUR);

		r = read(fd, &ie, sizeof(ie));
	}
	return TRUE;
}

static int _vconf_kdb_noti_init(void)
{
	GIOChannel *gio;
	int ret = 0;

	pthread_mutex_lock(&_kdb_inoti_fd_mutex);

	if (0 < _kdb_inoti_fd) {
		ERR("Error: invalid _kdb_inoti_fd");
		pthread_mutex_unlock(&_kdb_inoti_fd_mutex);
		return VCONF_ERROR;
	}
	_kdb_inoti_fd = inotify_init();
	if (_kdb_inoti_fd == -1) {
		char err_buf[100] = { 0, };
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("inotify init: %s", err_buf);
		pthread_mutex_unlock(&_kdb_inoti_fd_mutex);
		return VCONF_ERROR;
	}

	ret = fcntl(_kdb_inoti_fd, F_SETFD, FD_CLOEXEC);
	if (ret < 0) {
		char err_buf[100] = { 0, };
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("inotify init: %s", err_buf);
		pthread_mutex_unlock(&_kdb_inoti_fd_mutex);
		return VCONF_ERROR;
	}

	ret = fcntl(_kdb_inoti_fd, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		char err_buf[100] = { 0, };
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("inotify init: %s", err_buf);
		pthread_mutex_unlock(&_kdb_inoti_fd_mutex);
		return VCONF_ERROR;
	}

	pthread_mutex_unlock(&_kdb_inoti_fd_mutex);

	gio = g_io_channel_unix_new(_kdb_inoti_fd);
	retvm_if(gio == NULL, -1, "Error: create a new GIOChannel");

	g_io_channel_set_flags(gio, G_IO_FLAG_NONBLOCK, NULL);

	_kdb_handler = g_io_create_watch(gio, G_IO_IN);
	g_source_set_callback(_kdb_handler, (GSourceFunc) _vconf_kdb_gio_cb, NULL, NULL);
	g_source_attach(_kdb_handler, NULL);
	g_io_channel_unref(gio);
	g_source_unref(_kdb_handler);

	return VCONF_OK;
}

	int
_vconf_kdb_add_notify(const char *keyname, vconf_callback_fn cb, void *data)
{
	char path[VCONF_KEY_PATH_LEN];
	int wd;
	struct noti_node t, *n;
	char err_buf[ERR_LEN] = { 0, };
	int ret = 0;
	GList *list = NULL;
	int func_ret = VCONF_OK;

	retvm_if((keyname == NULL || cb == NULL), VCONF_ERROR,
			"_vconf_kdb_add_notify : Invalid argument - keyname(%s) cb(%p)",
			keyname, cb);

	if (_kdb_inoti_fd <= 0)
		if (_vconf_kdb_noti_init())
			return VCONF_ERROR;

	ret = _vconf_get_key_path((char*)keyname, path);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "Invalid argument: key is not valid");

	if (0 != access(path, F_OK)) {
		if (errno == ENOENT) {
			ERR("_vconf_kdb_add_notify : Key(%s) does not exist", keyname);
			return VCONF_ERROR;
		}
	}

	wd = inotify_add_watch(_kdb_inoti_fd, path, INOTY_EVENT_MASK);
	if (wd == -1) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("_vconf_kdb_add_notify : add noti(%s)", err_buf);
		return VCONF_ERROR;
	}

	t.wd = wd;
	t.cb = cb;

	pthread_mutex_lock(&_kdb_g_ns_mutex);

	list = g_list_find_custom(g_notilist, &t, (GCompareFunc)_vconf_inoti_comp_with_wd_cb);
	if (list) {
		ERR("_vconf_kdb_add_notify : key(%s) has same callback(%p)", keyname, cb);
		errno = EALREADY;
		func_ret = VCONF_ERROR;
		goto out_func;
	}

	n = calloc(1, sizeof(noti_node_s));
	if (n == NULL) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("_vconf_kdb_add_notify : add noti(%s)", err_buf);
		func_ret = VCONF_ERROR;
		goto out_func;
	}

	n->wd = wd;
	n->keyname = strndup(keyname, VCONF_KEY_PATH_LEN);
	n->cb_data = data;
	n->cb = cb;

	g_notilist = g_list_append(g_notilist, n);
	if(!g_notilist) {
		ERR("g_list_append fail");
	}

	INFO("cb(%p) is added for %s. tot cb cnt : %d\n", cb, n->keyname, g_list_length(g_notilist));

out_func:
	pthread_mutex_unlock(&_kdb_g_ns_mutex);

	return func_ret;
}

	int
_vconf_kdb_del_notify(const char *keyname, vconf_callback_fn cb)
{
	int wd = 0;
	int r = 0;
	struct noti_node *n = NULL;
	struct noti_node t;
	char path[VCONF_KEY_PATH_LEN] = { 0, };
	char err_buf[ERR_LEN] = { 0, };
	int del = 0;
	int ret = 0;
	int func_ret = VCONF_OK;
	GList *noti_list;

	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname(%s)", keyname);
	retvm_if(_kdb_inoti_fd == 0, VCONF_ERROR, "Invalid operation: not exist anything for inotify");

	ret = _vconf_get_key_path((char*)keyname, path);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "Invalid argument: key is not valid");

	/* get wd */
	wd = inotify_add_watch(_kdb_inoti_fd, path, INOTY_EVENT_MASK);
	if (wd == -1) {
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("Error: inotify_add_watch() [%s]: %s", path, err_buf);
		return VCONF_ERROR;
	}

	pthread_mutex_lock(&_kdb_g_ns_mutex);

	t.wd = wd;
	t.cb = cb;

	noti_list = g_list_find_custom(g_notilist, &t, (GCompareFunc)_vconf_inoti_comp_with_wd_cb);
	if(noti_list) {
		del++;

		n = noti_list->data;
		g_notilist = g_list_remove(g_notilist, n);
		g_free(n->keyname);
		g_free(n);

		INFO("key(%s) cb is removed. remained noti list total length(%d)",
				keyname, g_list_length(g_notilist));
	}

	noti_list = NULL;
	noti_list = g_list_find_custom(g_notilist, &t, (GCompareFunc)_vconf_inoti_comp_with_wd);
	if(noti_list == NULL) {
		//INFO("all noti for keyname(%s)/wd(%d) is removed", keyname, wd);

		r = inotify_rm_watch(_kdb_inoti_fd, wd);
		if(r == -1) {
			strerror_r(errno, err_buf, sizeof(err_buf));
			ERR("Error: inotify_rm_watch [%s]: %s", keyname, err_buf);
			func_ret = VCONF_ERROR;
		}
	}

	if(g_list_length(g_notilist) == 0) {
		close(_kdb_inoti_fd);
		_kdb_inoti_fd = 0;

		g_source_destroy(_kdb_handler);
		_kdb_handler = NULL;

		g_list_free(g_notilist);
		g_notilist = NULL;

		INFO("all noti list is freed");
	}

	pthread_mutex_unlock(&_kdb_g_ns_mutex);

	if(del == 0) {
		ERR("Error: nothing deleted");
		errno = ENOENT;
		func_ret = VCONF_ERROR;
	}

	return func_ret;
}
