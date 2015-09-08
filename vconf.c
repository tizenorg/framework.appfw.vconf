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

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "vconf-internals.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <ctype.h>
#include <string.h>

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#define VCONF_ERROR_RETRY_CNT 5
#define VCONF_ERROR_RETRY_SLEEP_UTIME 10000

#define VCONF_NOT_INITIALIZED (access("/tmp/vconf-initialized", F_OK) == -1)

#ifdef VCONF_USE_SQLFS_TRANSACTION
int IN_SBOX=0;

#define VCONF_MOUNT_PATH "/opt/var/kdb/db"
#define VCONF_MOUNT_PATH_CHECK \
do{\
	if(!IN_SBOX) \
      IN_SBOX = access("/opt/var/kdb/kdb_first_boot", F_OK) + 2; \
   	if(2==IN_SBOX) return 0;\
}while(0)

__thread int is_transaction;
#endif

#define VCONF_LCK_FILE_FMT "%s/.%s.lck"
#define LCK_FILE_GRP 5000

static int g_posix_errno;
static int g_vconf_errno;

#ifdef VCONF_TIMECHECK
double correction, startT;

double set_start_time(void)
{
	struct timeval tv;
	double curtime;

	gettimeofday(&tv, NULL);
	curtime = tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
	return curtime;
}

double exec_time(double start)
{
	double end = set_start_time();
	return (end - start - correction);
}

int init_time(void)
{
	double temp_t;
	temp_t = set_start_time();
	correction = exec_time(temp_t);

	return 0;
}
#endif

int _vconf_keynode_set_keyname(keynode_t *keynode, const char *keyname)
{
	if (keynode->keyname) free(keynode->keyname);
	keynode->keyname = strndup(keyname, VCONF_KEY_PATH_LEN);
	retvm_if(keynode->keyname == NULL, VCONF_ERROR, "strndup Fails");
	return VCONF_OK;
}

static inline void _vconf_keynode_set_dir(keynode_t *keynode)
{
	keynode->type = VCONF_TYPE_DIR;
}

static inline void _vconf_keynode_set_value_int(keynode_t *keynode, const int value)
{
	keynode->type = VCONF_TYPE_INT;
	keynode->value.i = value;
}

static inline void _vconf_keynode_set_value_bool(keynode_t *keynode, const int value)
{
	keynode->type = VCONF_TYPE_BOOL;
	keynode->value.b = !!value;
}

static inline void _vconf_keynode_set_value_dbl(keynode_t *keynode, const double value)
{
	keynode->type = VCONF_TYPE_DOUBLE;
	keynode->value.d = value;
}

static inline void _vconf_keynode_set_value_str(keynode_t *keynode, const char *value)
{
	keynode->type = VCONF_TYPE_STRING;
	keynode->value.s = strdup(value);
}

inline void _vconf_keynode_set_null(keynode_t *keynode)
{
	keynode->type = VCONF_TYPE_NONE;
	//keynode->value.d = NULL;
}

static inline keynode_t *_vconf_keynode_next(keynode_t *keynode)
{
	return keynode->next;
}

inline keynode_t *_vconf_keynode_new(void)
{
	keynode_t *keynode;
	keynode = calloc(1, sizeof(keynode_t));

	return keynode;
}

inline void _vconf_keynode_free(keynode_t *keynode)
{
	if(keynode) {
		if (keynode->keyname)
			free(keynode->keyname);
		if (keynode->type == VCONF_TYPE_STRING && keynode->value.s)
			free(keynode->value.s);
		free(keynode);
	}
}

static inline keynode_t *_vconf_keylist_headnode(keylist_t *keylist)
{
	return keylist->head;
}

static keynode_t *_vconf_keylist_lookup(keylist_t *keylist, const char *keyname,
				 keynode_t **before_keynode)
{
	keynode_t *found_node, *temp_node = NULL;

	found_node = _vconf_keylist_headnode(keylist);

	while (found_node) {
		if(found_node->keyname == NULL) {
			ERR("key node has null keyname");
			return NULL;
		}

		if (!strncmp(keyname, found_node->keyname, strlen(keyname))) {
			if (before_keynode) {
					*before_keynode = temp_node;
			}
			return found_node;
		}

		temp_node = found_node;
		found_node = _vconf_keynode_next(found_node);
	}
	return NULL;
}

/*
 * This function get Key name of the keynode.
 * @param[in] keynode The Key
 * @return Key Name of the keynode
 */
API char *vconf_keynode_get_name(keynode_t *keynode)
{
	retvm_if(keynode == NULL, NULL, "Invalid argument: keynode is NULL");
	retvm_if(keynode->keyname == NULL, NULL, "The name of keynode is NULL");

	return keynode->keyname;
}

/*
 * This function get value type of the keynode.
 * @param[in] keynode The Key
 * @return Type of the keynode
 */
API int vconf_keynode_get_type(keynode_t *keynode)
{
	retvm_if(keynode == NULL, VCONF_ERROR, "Invalid argument: keynode is NULL");

	return keynode->type;
}

API int vconf_keynode_get_int(keynode_t *keynode)
{
	retvm_if(keynode == NULL, VCONF_ERROR, "Invalid argument: keynode is NULL");
	retvm_if(keynode->type != VCONF_TYPE_INT, VCONF_ERROR,
		 "The type(%d) of keynode(%s) is not INT", keynode->type, keynode->keyname);

	return keynode->value.i;
}

API double vconf_keynode_get_dbl(keynode_t *keynode)
{
	retvm_if(keynode == NULL, -1.0, "Invalid argument: keynode is NULL");
	retvm_if(keynode->type != VCONF_TYPE_DOUBLE, -1.0,
		 "The type(%d) of keynode(%s) is not DBL", keynode->type, keynode->keyname);

	return keynode->value.d;
}

/*
 * This function get Boolean value of the keynode.
 * @param[in] keynode The Key
 * @return Boolean value, -1 on error
 */
API int vconf_keynode_get_bool(keynode_t *keynode)
{
	retvm_if(keynode == NULL, VCONF_ERROR, "Invalid argument: keynode is NULL");
	retvm_if(keynode->type != VCONF_TYPE_BOOL, VCONF_ERROR,
		 "The type(%d) of keynode(%s) is not BOOL", keynode->type, keynode->keyname);

	return !!(keynode->value.b);
}

/*
 * This function get String value of the keynode.
 * @param[in] keynode The Key
 * @return String value, NULL on error
 */
API char *vconf_keynode_get_str(keynode_t *keynode)
{
	retvm_if(keynode == NULL, NULL, "Invalid argument: keynode is NULL");
	retvm_if(keynode->type != VCONF_TYPE_STRING, NULL,
		"The type(%d) of keynode(%s) is not STR", keynode->type, keynode->keyname);

	return keynode->value.s;
}

/*
 * Allocate, initialize and return a new Keylist object.
 * @return The pointer of New keylist, NULL on error
 */
API keylist_t *vconf_keylist_new(void)
{
	keylist_t *keylist;
	keylist = calloc(1, sizeof(keylist_t));

	return keylist;
}

/*
 * This function rewinds the KeyList internal cursor.
 * @param[in] keylist Key List
 * @return 0 on success, -1 on error
 */
API int vconf_keylist_rewind(keylist_t *keylist)
{
	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");

	keylist->cursor = NULL;

	return 0;
}

/*
 * A destructor for Keylist objects.
 * @param[in] keylist Key List
 * @return 0 on success, -1 on error
 */
API int vconf_keylist_free(keylist_t *keylist)
{
	keynode_t *keynode, *temp;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");

	if (keylist->num) {
		keynode = _vconf_keylist_headnode(keylist);
		while (keynode) {
			temp = _vconf_keynode_next(keynode);
			_vconf_keynode_free(keynode);
			keynode = temp;
		}
	}
	free(keylist);
	return 0;
}

/*
 * This function look for a Keynode contained in keylist that matches keyname.
 * @param[in] keylist Key List
 * @param[in] keyname Key to find
 * @param[out] return_node pointer of keynode to set
 * @return Type of the found key
 */
API int
vconf_keylist_lookup(keylist_t *keylist,
		     const char *keyname, keynode_t **return_node)
{
	keynode_t *found_node;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");
	retvm_if(return_node == NULL, VCONF_ERROR, "Invalid argument: return_node is NULL");

	found_node = _vconf_keylist_lookup(keylist, keyname, NULL);
	if (NULL == found_node)
		return 0;

	if (return_node)
		*return_node = found_node;
	return found_node->type;
}

/*
 * This function returns the next Key in a Keylist.
 * Next key is known by the keylist internal cursor.
 * @param[in] keylist Key List
 * @return The next Keynode, NULL on error
 */
API keynode_t *vconf_keylist_nextnode(keylist_t *keylist)
{
	retvm_if(keylist == NULL, NULL, "Invalid argument: keylist is NULL");

	if (keylist->cursor)
		keylist->cursor = _vconf_keynode_next(keylist->cursor);
	else
		keylist->cursor = keylist->head;

	return keylist->cursor;
}

/*
 * This function appends a new Keynode included integer value to the keylist.
 * If same keyname exist, the keynode will change.
 * @param[in] keylist Key List
 * @param[in] keyname Key
 * @param[in] value The integer value
 * @return Number of keynode included in the keylist, -1 on error
 */
API int
vconf_keylist_add_int(keylist_t *keylist, const char *keyname, const int value)
{
	keynode_t *keynode = NULL, *addition = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if ((keynode = _vconf_keylist_lookup(keylist, keyname, NULL))) {
		_vconf_keynode_set_value_int(keynode, value);
		return keylist->num;
	}
	if ((keynode = _vconf_keylist_headnode(keylist)))
		while (_vconf_keynode_next(keynode))
			keynode = _vconf_keynode_next(keynode);

	addition = calloc(1, sizeof(keynode_t));
	retvm_if(addition == NULL, VCONF_ERROR, "(maybe)not enought memory");
	if (!_vconf_keynode_set_keyname(addition, keyname)) {
		_vconf_keynode_set_value_int(addition, value);
		if (keylist->head && NULL != keynode)
			keynode->next = addition;
		else
			keylist->head = addition;
		keylist->num += 1;
	} else {
		ERR("(maybe)not enought memory");
		free(addition), addition = NULL;
		return VCONF_ERROR;
	}

	return keylist->num;
}

/*
 * This function appends a new Keynode included boolean value to the keylist.
 * If same keyname exist, the keynode will change.
 * @param[in] keylist Key List
 * @param[in] keyname Key
 * @param[in] value The boolean value
 * @return Number of keynode included in the keylist, -1 on error
 */
API int
vconf_keylist_add_bool(keylist_t *keylist, const char *keyname, const int value)
{
	keynode_t *keynode = NULL, *addition = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if ((keynode = _vconf_keylist_lookup(keylist, keyname, NULL))) {
		_vconf_keynode_set_value_bool(keynode, value);
		return keylist->num;
	}
	if ((keynode = _vconf_keylist_headnode(keylist)))
		while (_vconf_keynode_next(keynode))
			keynode = _vconf_keynode_next(keynode);

	addition = calloc(1, sizeof(keynode_t));
	retvm_if(addition == NULL, VCONF_ERROR, "(maybe)not enought memory");
	if (!_vconf_keynode_set_keyname(addition, keyname)) {
		_vconf_keynode_set_value_bool(addition, value);
		if (keylist->head && NULL != keynode)
			keynode->next = addition;
		else
			keylist->head = addition;
		keylist->num += 1;
	} else {
		ERR("(maybe)not enought memory");
		free(addition), addition = NULL;
		return VCONF_ERROR;
	}

	return keylist->num;
}

/*
 * This function appends a new Keynode included double value to the keylist.
 * If same keyname exist, the keynode will change.
 * @param[in] keylist Key List
 * @param[in] keyname Key
 * @param[in] value The double value
 * @return Number of keynode included in the keylist, -1 on error
 */
API int
vconf_keylist_add_dbl(keylist_t *keylist,
		      const char *keyname, const double value)
{
	keynode_t *keynode = NULL, *addition = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if ((keynode = _vconf_keylist_lookup(keylist, keyname, NULL))) {
		_vconf_keynode_set_value_dbl(keynode, value);
		return keylist->num;
	}
	if ((keynode = _vconf_keylist_headnode(keylist)))
		while (_vconf_keynode_next(keynode))
			keynode = _vconf_keynode_next(keynode);

	addition = calloc(1, sizeof(keynode_t));
	retvm_if(addition == NULL, VCONF_ERROR, "(maybe)not enought memory");
	if (!_vconf_keynode_set_keyname(addition, keyname)) {
		_vconf_keynode_set_value_dbl(addition, value);
		if (keylist->head && NULL != keynode)
			keynode->next = addition;
		else
			keylist->head = addition;
		keylist->num += 1;
	} else {
		ERR("(maybe)not enought memory");
		free(addition), addition = NULL;
		return VCONF_ERROR;
	}

	return keylist->num;
}

/*
 * This function appends a new Keynode included string value to the keylist.
 * If same keyname exist, the keynode will change.
 * @param[in] keylist Key List
 * @param[in] keyname Key
 * @param[in] value The pointer of string value
 * @return Number of keynode included in the keylist, -1 on error
 */
API int
vconf_keylist_add_str(keylist_t *keylist,
		      const char *keyname, const char *value)
{
	keynode_t *keynode = NULL, *addition = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if ((keynode = _vconf_keylist_lookup(keylist, keyname, NULL))) {
		if (VCONF_TYPE_STRING == keynode->type && keynode->value.s)
			free(keynode->value.s);
		_vconf_keynode_set_value_str(keynode, value);
		return keylist->num;
	}
	if (NULL != (keynode = _vconf_keylist_headnode(keylist)))
		while (_vconf_keynode_next(keynode))
			keynode = _vconf_keynode_next(keynode);

	addition = calloc(1, sizeof(keynode_t));
	retvm_if(addition == NULL, VCONF_ERROR, "(maybe)not enought memory");
	if (!_vconf_keynode_set_keyname(addition, keyname)) {
		_vconf_keynode_set_value_str(addition, value);
		if (keylist->head && NULL != keynode)
			keynode->next = addition;
		else
			keylist->head = addition;
		keylist->num += 1;
	} else {
		ERR("(maybe)not enought memory");
		free(addition), addition = NULL;
		return VCONF_ERROR;
	}

	return keylist->num;
}

/*
 * This function Appends a new Keynode to the keylist without value.
 * Use on vconf_get()
 * @param[in] keylist Key List
 * @param[in] keyname Key
 * @return Number of keynode included in the keylist, -1 on error
 */
API int vconf_keylist_add_null(keylist_t *keylist, const char *keyname)
{
	keynode_t *keynode = NULL, *addition = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if (NULL != (keynode = _vconf_keylist_lookup(keylist, keyname, NULL))) {
		keynode->type = 0;
		keynode->value.d = 0;
		return keylist->num;
	}
	if ((keynode = _vconf_keylist_headnode(keylist)))
		while (_vconf_keynode_next(keynode))
			keynode = _vconf_keynode_next(keynode);

	addition = calloc(1, sizeof(keynode_t));
	retvm_if(addition == NULL, VCONF_ERROR, "(maybe)not enought memory");
	if (!_vconf_keynode_set_keyname(addition, keyname)) {
		if (keylist->head && keynode)
			keynode->next = addition;
		else
			keylist->head = addition;
		keylist->num += 1;
	} else {
		ERR("(maybe)not enought memory");
		free(addition), addition = NULL;
		return VCONF_ERROR;
	}

	return keylist->num;
}

/*
 * This function remove the keynode that matches keyname.
 * @param[in] keylist the keylist included the keyname
 * @param[in] keyname key
 * @return 0 on success, -1(Invalid parameter), -2(Not exist keyname in keylist) on error
 */
API int vconf_keylist_del(keylist_t *keylist, const char *keyname)
{
	keynode_t *found_node, *before_node = NULL;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");
	retvm_if(keyname == NULL, VCONF_ERROR, "Invalid argument: keyname is NULL");

	if ((found_node = _vconf_keylist_lookup(keylist, keyname, &before_node))) {
		if(before_node) {
			before_node->next = found_node->next;
		} else {
			/* requested key is headnode of keylist */
			keylist->head = found_node->next;
		}

		_vconf_keynode_free(found_node);
	} else
		return VCONF_ERROR;

	return VCONF_OK;
}


int _vconf_get_key_prefix(const char *keyname, int *prefix)
{
	if (strncmp(keyname, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1) == 0) {
		*prefix = VCONF_BACKEND_DB;
	} else if (0 == strncmp(keyname, BACKEND_FILE_PREFIX, sizeof(BACKEND_FILE_PREFIX) - 1)) {
		*prefix = VCONF_BACKEND_FILE;
	} else if (0 == strncmp(keyname, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
		*prefix = VCONF_BACKEND_MEMORY;
	} else {
		ERR("Invalid argument: wrong prefix of key(%s)", keyname);
		*prefix = VCONF_BACKEND_NULL;
		return VCONF_ERROR_WRONG_PREFIX;
	}

	return VCONF_OK;
}

int _vconf_get_key_path(const char *keyname, char *path)
{
	const char *key = NULL;

	if(!keyname) {
		ERR("keyname is null");
		return VCONF_ERROR_WRONG_PREFIX;
	}

#ifdef COMBINE_FOLDER
	char convert_key[VCONF_KEY_PATH_LEN+1] = {0,};
	char *chrptr = NULL;

	strncpy(convert_key, keyname, VCONF_KEY_PATH_LEN);

	chrptr = strchr((const char*)convert_key, (int)'/');
	if(!chrptr) {
		ERR("wrong key path!");
		return VCONF_ERROR_WRONG_PREFIX;
	}
	chrptr = strchr((const char*)chrptr+1, (int)'/');
	while(chrptr) {
		convert_key[chrptr-convert_key] = '+';
		chrptr = strchr((const char*)chrptr+1, (int)'/');
	}
	key = (const char*)convert_key;
#else
	key = keyname;
#endif

	if (strncmp(key, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1) == 0) {
		snprintf(path, VCONF_KEY_PATH_LEN, "%s%s", BACKEND_SYSTEM_DIR, key);
	} else if (0 == strncmp(key, BACKEND_FILE_PREFIX, sizeof(BACKEND_FILE_PREFIX) - 1)) {
		snprintf(path, VCONF_KEY_PATH_LEN, "%s%s", BACKEND_SYSTEM_DIR, key);
	} else if (0 == strncmp(key, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
		snprintf(path, VCONF_KEY_PATH_LEN, "%s%s", BACKEND_MEMORY_DIR, key);
	} else {
		ERR("Invalid argument: wrong prefix of key(%s)", key);
		return VCONF_ERROR_WRONG_PREFIX;
	}

	return VCONF_OK;
}

#ifdef VCONF_USE_BACKUP_TRANSACTION
int _vconf_get_backup_path(const char *keyname, char *path)
{
	char key_buf[VCONF_KEY_PATH_LEN] = {0, };
	int i;

	for(i = 0; i<VCONF_KEY_PATH_LEN-1 && keyname[i] != '\0' ; i++) {
		if (keyname[i] == '/')
			key_buf[i] = '$';
		else
			key_buf[i] = keyname[i];
	}

	snprintf(path, VCONF_KEY_PATH_LEN, "%s%s%s%s", BACKEND_SYSTEM_DIR, BACKEND_DB_PREFIX, ".backup/", key_buf);

	return VCONF_OK;
}
#endif

#ifndef DISABLE_RUNTIME_KEY_CREATION
static int _vconf_set_key_check_parent_dir(const char* path)
{
	int exists = 0;
	struct stat stat_info;
	char* parent;
	char path_buf[VCONF_KEY_PATH_LEN] = {0,};
	int ret = 0;

	mode_t dir_mode =  0664 | 0111;

	parent = strrchr(path, '/');
	strncpy(path_buf, path, parent-path);
	path_buf[parent-path]=0;

	exists = stat(path_buf,&stat_info);
	if(exists){
		if(mkdir(path_buf, dir_mode) != 0) {
			if(errno == ENOENT) {
				ret = _vconf_set_key_check_parent_dir((const char*)path_buf);
				if(ret != VCONF_OK) return ret;
				if(mkdir(path_buf, dir_mode) != 0) {
					ERR("mkdir error(%d)", errno);
					return VCONF_ERROR;
				}
			}
		}
	}

	return VCONF_OK;
}

static int _vconf_set_key_creation(const char* path)
{
	int fd;
	mode_t temp;
	temp = umask(0000);
	fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	umask(temp);
	if(fd == -1) {
		ERR("open(rdwr,create) error: %d, %s", errno, strerror(errno));
		return VCONF_ERROR;
	}
	close(fd);

	return VCONF_OK;
}
#endif

static int _vconf_set_file_lock(int fd, short type)
{
	struct flock l;

	l.l_type = type;
	l.l_start= 0;		/*Start at begin*/
	l.l_whence = SEEK_SET;
	l.l_len = 0;		/*Do it with whole file*/

	return fcntl(fd, F_SETLKW, &l);
}

static int _vconf_set_read_lock(int fd)
{
	return _vconf_set_file_lock(fd, F_RDLCK);
}

static int _vconf_set_write_lock(int fd)
{
	return _vconf_set_file_lock(fd, F_WRLCK);
}

static int _vconf_set_unlock(int fd)
{
	return _vconf_set_file_lock(fd, F_UNLCK);
}

#ifdef VCONF_USE_SQLFS_TRANSACTION
static void _vconf_acquire_transaction_delay(int ms)
{
    struct timeval timeout;
    timeout.tv_sec = ms / 1000 ;
    timeout.tv_usec = 1000 * ( ms % 1000 );

    select(0, 0, 0, 0, &timeout);
}

static int _vconf_db_begin_transaction()
{
	pid_t value;
	VCONF_MOUNT_PATH_CHECK;

	if(is_transaction == 0) {
		value = getpid();
transaction_retry:
		if(setxattr(VCONF_MOUNT_PATH, "full_db_transaction_start", &value, sizeof(value), 0) == -1)
		{
			//ERR("someone access vconf. retrying...(%d)", errno);
			_vconf_acquire_transaction_delay(50);
			goto transaction_retry;
		} else {
			is_transaction++;
		}
	} else {
		is_transaction++;
	}

	return VCONF_OK;
}

static int _vconf_db_commit_transaction()
{
	pid_t value;
	VCONF_MOUNT_PATH_CHECK;

	retv_if(is_transaction <= 0, VCONF_ERROR);

	if(is_transaction == 1) {
		value = getpid();
transaction_retry:
		if(setxattr(VCONF_MOUNT_PATH, "full_db_transaction_stop", &value, sizeof(value), 0) == -1)
		{
			//ERR("setxattr failed. retrying...(%d)", errno);
			_vconf_acquire_transaction_delay(50);
			goto transaction_retry;
		} else {
			is_transaction = 0;
		}
	} else {
		is_transaction--;
	}

	return 0;
}

static int _vconf_db_rollback_transaction()
{
	pid_t value;
	VCONF_MOUNT_PATH_CHECK;

	retv_if(is_transaction <= 0, VCONF_ERROR);

	if(is_transaction == 1) {
		value = getpid();
transaction_retry:
		if(setxattr(VCONF_MOUNT_PATH, "full_db_transaction_rb", &value, sizeof(value), 0) == -1)
		{
			//ERR("setxattr failed. retrying...(%d)", errno);
			_vconf_acquire_transaction_delay(50);
			goto transaction_retry;
		} else {
			is_transaction = 0;
		}
	} else {
		is_transaction --;
	}

	return 0;
}
#endif //VCONF_USE_SQLFS_TRANSACTION

#ifdef VCONF_USE_BACKUP_TRANSACTION

static int _vconf_backup_check(char* vconf_path, char* backup_path)
{
	int ret = VCONF_OK;
	int vconf_fd = -1;
	int backup_fd = -1;
	char file_buf[BUF_LEN] = { 0, };
	char err_buf[100] = { 0, };
	int complete_mark = 0;
	int read_len;
	int err_no = 0;
	int is_recovered = 0;
	int file_size;

	if (access(vconf_path, F_OK) == -1 || access(backup_path, F_OK) == -1)
		return ret;

	vconf_fd = open(vconf_path, O_WRONLY);
	if (vconf_fd == -1) {
		ERR("open %s failed", vconf_path);
		ret = VCONF_ERROR_FILE_OPEN;
		err_no = errno;
		goto out_close;
	}

	if(_vconf_set_write_lock(vconf_fd) == -1) {
		ERR("set write lock %s failed", vconf_path);
		ret = VCONF_ERROR_FILE_LOCK;
		err_no = errno;
		goto out_close;
	}

	backup_fd = open(backup_path, O_RDONLY);
	if (backup_fd == -1) {
		ERR("open %s failed", backup_path);
		ret = VCONF_ERROR_FILE_OPEN;
		err_no = errno;
		goto out_unlock;
	}

	file_size = lseek(backup_fd, 0, SEEK_END);
	if (file_size == -1) {
		ERR("seek end %s failed", backup_path);
		ret = VCONF_ERROR_FILE_SEEK;
		err_no = errno;
		goto out_unlock;
	} else if (file_size < FILE_ATOMIC_GUARANTEE_SIZE + VCONF_BACKUP_COMP_MARK_SIZE) {
		goto out_unlock;
	}

	if (lseek(backup_fd, 0, SEEK_SET) != 0) {
		ERR("seek %s failed", backup_path);
		ret = VCONF_ERROR_FILE_SEEK;
		err_no = errno;
		goto out_unlock;
	}

	read_len = read(backup_fd, &complete_mark, sizeof(int));
	if (read_len < sizeof(int))
		goto out_unlock;

	if (complete_mark != VCONF_BACKUP_COMPLETE_MARK)
		goto out_unlock;

	if (ftruncate(vconf_fd, 0) == -1) {
		ERR("truncate %s failed", vconf_path);
		ret = VCONF_ERROR_FILE_TRUNCATE;
		err_no = errno;
		goto out_unlock;
	}

	while ( (read_len = read(backup_fd, file_buf, BUF_LEN )) >0) {
		if(write(vconf_fd, file_buf, read_len) != read_len) {
			ERR("write %s failed", backup_path);
			ret = VCONF_ERROR_FILE_WRITE;
			err_no = errno;
			goto out_unlock;
		}
	}

	is_recovered = 1;

out_unlock :
	if(_vconf_set_unlock(vconf_fd)==-1) {
		ERR("unset write lock %s failed", vconf_path);
		ret = VCONF_ERROR_FILE_LOCK;
		err_no = errno;
		goto out_close;
	}

out_close:
	if (vconf_fd != -1)
		close(vconf_fd);
	if(backup_fd != -1)
		close(backup_fd);

	if(ret == VCONF_OK) {
		if(remove(backup_path) == -1) {
			ret = VCONF_ERROR_FILE_REMOVE;
			err_no = errno;
		}
	}

	if(ret != VCONF_OK) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_backup_check failed %d (%d / %s)\n", ret, err_no, err_buf);
	} else if (is_recovered)
		WARN("vconf(%s) successfully recovered and backup file is removed", vconf_path, backup_path);
	else
		WARN("vconf(%s)'s non-complete backup file is removed", vconf_path, backup_path);

	return ret;
}

static int _vconf_backup_write_str(char* backup_path, char* value)
{
	int ret = VCONF_OK;
	char err_buf[100] = { 0, };
	int err_no = 0;
	int backup_fd = -1;
	int write_len = 0;
	int complete_mark = VCONF_BACKUP_COMPLETE_MARK;
	int empty_mark = 0;
	int vconf_type_str = VCONF_TYPE_STRING;
	int result;

	if( (backup_fd = open(backup_path, O_CREAT|O_EXCL|O_WRONLY, 0666)) == -1 ) {
		ERR("open %s failed", backup_path);
		ret = VCONF_ERROR_FILE_OPEN;
		err_no = errno;
		goto out_close;
	}

	if ((result = write(backup_fd, (void *)&empty_mark, sizeof(int))) != sizeof(int)) {
		ERR("write empty mark of %s failed %d", backup_path, result);
		ret = VCONF_ERROR_FILE_WRITE;
		err_no = errno;
		goto out_close;
	}

	if ((result = write(backup_fd, (void *)&vconf_type_str, sizeof(int))) != sizeof(int)) {
		ERR("write type of %s failed %d", backup_path, result);
		ret = VCONF_ERROR_FILE_WRITE;
		err_no = errno;
		goto out_close;
	}

	write_len = strlen(value);
	if((result = write(backup_fd, value, write_len)) != write_len) {
		ERR("write %s failed %d", backup_path, result);
		ret = VCONF_ERROR_FILE_WRITE;
		err_no = errno;
		goto out_close;

	}

	if (fdatasync(backup_fd) == -1) {
		ERR("sync %s failed", backup_path);
		ret = VCONF_ERROR_FILE_SYNC;
		err_no = errno;
		goto out_close;
	}

	if (lseek(backup_fd, 0, SEEK_SET) != 0) {
		ERR("seek %s failed", backup_path);
		ret = VCONF_ERROR_FILE_SEEK;
		err_no = errno;
		goto out_close;
	}

	if((result = write(backup_fd, (void *)&complete_mark, sizeof(int))) != sizeof(int)) {
		ERR("write complete mark of %s failed %d", backup_path, result);
		ret = VCONF_ERROR_FILE_WRITE;
		err_no = errno;
		goto out_close;
	}

	if (fdatasync(backup_fd) == -1) {
		ERR("sync %s failed", backup_path);
		ret = VCONF_ERROR_FILE_SYNC;
		err_no = errno;
		goto out_close;
	}

out_close:
	if(backup_fd != -1)
		close(backup_fd);

	if(ret != VCONF_OK) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_backup_write_str failed %d (%d / %s)\n", ret, err_no, err_buf);
	}

	return ret;

}

static int _vconf_backup_commit(char* backup_path)
{
	int ret = VCONF_OK;
	char err_buf[100] = { 0, };
	int err_no = 0;

	if(remove(backup_path) == -1) {
		ERR("remove %s failed", backup_path);
		ret = VCONF_ERROR_FILE_REMOVE;
		err_no = errno;
	}

	if(ret != VCONF_OK) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_backup_commit failed %d (%d / %s)\n", ret, err_no, err_buf);
	}

	return ret;

}
#endif // VCONF_USE_BACKUP_TRANSACTION

static int _vconf_set_key_filesys(keynode_t *keynode, int prefix, int *io_errno)
{
	char path[VCONF_KEY_PATH_LEN] = {0,};
	FILE *fp = NULL;
	int ret = -1;
	int func_ret = VCONF_OK;
	int err_no = 0;
	char err_buf[100] = { 0, };
	int is_write_error = 0;

#ifdef VCONF_USE_BACKUP_TRANSACTION
	char backup_path[VCONF_KEY_PATH_LEN] = {0,};
	int is_backup_need = 0;
	int file_size = 0;
#endif

#ifdef VCONF_USE_LOCK_FILE
	char tmp[VCONF_KEY_PATH_LEN] = {0,};
	char lck[VCONF_KEY_PATH_LEN] = {0,};
	char *pCh;
	FILE *lckfp = NULL;
#endif

	errno = 0;

	ret = _vconf_get_key_path(keynode->keyname, path);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_CHECK_IS_INITIALIZED
	if(prefix == VCONF_BACKEND_MEMORY && VCONF_NOT_INITIALIZED) {
		func_ret = VCONF_ERROR_NOT_INITIALIZED;
		goto out_return;
	}
#endif

#ifdef VCONF_USE_BACKUP_TRANSACTION
	if(prefix == VCONF_BACKEND_DB && keynode->type == VCONF_TYPE_STRING) {
		_vconf_get_backup_path(keynode->keyname, backup_path);
		ret = _vconf_backup_check(path, backup_path);
		if(ret != VCONF_OK) {
			func_ret = ret;
			err_no = errno;
			goto out_return;
		}
	}
#endif

	if( (fp = fopen(path, "r+")) == NULL ) {
		func_ret = VCONF_ERROR_FILE_OPEN;
		err_no = errno;
		goto out_return;
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if (prefix != VCONF_BACKEND_DB)
#endif
	{
#ifdef VCONF_USE_LOCK_FILE
		pCh = strrchr(path, '/');
		strncat(tmp, path, pCh - path);
		snprintf(lck, VCONF_KEY_PATH_LEN, VCONF_LCK_FILE_FMT, tmp, pCh+1);

#ifndef DISABLE_RUNTIME_KEY_CREATION
		int exists = 1;
		if(access(lck, F_OK) == -1) exists = 0;
#endif

		if( (lckfp = fopen(lck, "w+")) == NULL ) {
			func_ret = VCONF_ERROR_FILE_OPEN;
			err_no = errno;
			goto out_close;
		}

#ifndef DISABLE_RUNTIME_KEY_CREATION
		if(exists == 0) {
			if (fchmod(fileno(lckfp), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) == -1) {
				func_ret = VCONF_ERROR_FILE_CHMOD;
				err_no = errno;
				goto out_close;
			}
			if (fchown(fileno(lckfp), 0, LCK_FILE_GRP) == -1) {
				func_ret = VCONF_ERROR_FILE_CHMOD;
				err_no = errno;
				goto out_close;
			}
		}
#endif
		ret = _vconf_set_write_lock(fileno(lckfp));
#else
		ret = _vconf_set_write_lock(fileno(fp));
#endif
		if(ret == -1) {
			func_ret = VCONF_ERROR_FILE_LOCK;
			err_no = errno;
			goto out_close;
		}
	}

#ifdef VCONF_USE_BACKUP_TRANSACTION
	if(prefix == VCONF_BACKEND_DB && keynode->type == VCONF_TYPE_STRING) {
		file_size = VCONF_TYPE_SIZE + strlen(keynode->value.s);
		if (file_size > FILE_ATOMIC_GUARANTEE_SIZE) {
			is_backup_need = 1;

			ret = _vconf_backup_write_str(backup_path, keynode->value.s);
			if(ret != VCONF_OK) {
				func_ret = ret;
				err_no = errno;
				goto out_unlock;
			} else
				WARN("vconf backup file for(%s) is created. file size(%d)", path, file_size);
		}
	}
#endif

	if (ftruncate(fileno(fp), 0) == -1) {
		func_ret = VCONF_ERROR_FILE_TRUNCATE;
		err_no = errno;
		goto out_unlock;
	}

	/* write key type */
	ret = fwrite((void *)&(keynode->type), sizeof(int), 1, fp);
	if(ret <= 0)
	{
		if(errno) {
			err_no = errno;
		} else {
			errno = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_WRITE;
		goto out_unlock;
	}

	/* write key value */
	switch(keynode->type)
	{
		case VCONF_TYPE_INT:
			ret = fwrite((void *)&(keynode->value.i), sizeof(int), 1, fp);
			if(ret <= 0) is_write_error = 1;
			break;
		case VCONF_TYPE_DOUBLE:
			ret = fwrite((void *)&(keynode->value.d), sizeof(double), 1, fp);
			if(ret <= 0) is_write_error = 1;
			break;
		case VCONF_TYPE_BOOL:
			ret = fwrite((void *)&(keynode->value.b), sizeof(int), 1, fp);
			if(ret <= 0) is_write_error = 1;
			break;
		case VCONF_TYPE_STRING:
			ret = fprintf(fp,"%s",keynode->value.s);
			if(ret < strlen(keynode->value.s)) is_write_error = 1;
			//ret = fwrite((void *)keynode->value.s, sizeof(char), strlen(keynode->value.s), fp);
			break;
		default :
			func_ret = VCONF_ERROR_WRONG_TYPE;
			goto out_unlock;
	}
	if(is_write_error)
	{
		if(errno) {
			err_no = errno;
		} else {
			errno = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_WRITE;
		goto out_unlock;
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_FILE)
#else
	if(prefix == VCONF_BACKEND_FILE || prefix == VCONF_BACKEND_DB)
#endif
	{
		fflush(fp);

		ret = fdatasync(fileno(fp));
		if(ret == -1) {
			err_no = errno;
			func_ret = VCONF_ERROR_FILE_SYNC;
		}
	}

#ifdef VCONF_USE_BACKUP_TRANSACTION
	if (is_backup_need) {
		ret = _vconf_backup_commit(backup_path);
		if(ret != VCONF_OK) {
			func_ret = ret;
			err_no = errno;
			goto out_unlock;
		}
	}
#endif

out_unlock :
#ifdef VCONF_USE_SQLFS_TRANSACTION
	if (prefix != VCONF_BACKEND_DB)
#endif
	{
#ifdef VCONF_USE_LOCK_FILE
		ret = _vconf_set_unlock(fileno(lckfp));
#else
		ret = _vconf_set_unlock(fileno(fp));
#endif
		if(ret == -1) {
			func_ret = VCONF_ERROR_FILE_LOCK;
			err_no = errno;
			goto out_close;
		}
	}
out_close :
	fclose(fp);
#ifdef VCONF_USE_LOCK_FILE
	if(lckfp) fclose(lckfp);
#endif

out_return :
	if(err_no != 0) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_set_key_filesys(%d-%s) step(%d) failed(%d / %s)\n", keynode->type, keynode->keyname, func_ret, err_no, err_buf);
	}

	*io_errno = err_no;

	return func_ret;
}

static int _vconf_set_key(keynode_t *keynode)
{
	int func_ret = VCONF_OK;
	int ret = 0;
	int is_busy_err = 0;
	int retry = -1;
	int prefix = 0;
	int io_errno = 0;

	ret = _vconf_get_key_prefix(keynode->keyname, &prefix);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		_vconf_db_begin_transaction();
	}
#endif

	while((ret = _vconf_set_key_filesys(keynode, prefix, &io_errno)) != VCONF_OK)
	{
		is_busy_err = 0;
		retry++;

		g_posix_errno = io_errno;
		g_vconf_errno = ret;

#ifdef VCONF_CHECK_INITIALIZED
		if(VCONF_NOT_INITIALIZED)
		{
			ERR("%s : vconf is not initialized\n", keynode->keyname);
			is_busy_err = 1;
		}
		else if(ret == VCONF_ERROR_FILE_OPEN)
#else

		if(ret == VCONF_ERROR_FILE_OPEN)
#endif
		{
			switch (io_errno)
			{
				/* file is not exist, make path */
#ifndef DISABLE_RUNTIME_KEY_CREATION
				case EFAULT :
				case ENOENT :
				{
					int rc = 0;
					char path[VCONF_KEY_PATH_LEN] = {0,};
					rc = _vconf_get_key_path(keynode->keyname, path);
					if(rc != VCONF_OK) {
						ERR("_vconf_get_key_path error");
						break;
					}

					ret = _vconf_set_key_check_parent_dir(path);
					if(rc != VCONF_OK) {
						ERR("_vconf_set_key_check_parent_dir error : %s", path);
						break;
					}

					ret = _vconf_set_key_creation(path);
					if(rc != VCONF_OK) {
						ERR("_vconf_set_key_creation error : %s", path);
						break;
					}
					INFO("%s key is created", keynode->keyname);
				}
#endif
				case EAGAIN :
				case EMFILE :
				case ENFILE :
				case ETXTBSY :
				{
					is_busy_err = 1;
				}
			}
		}
		else if (ret == VCONF_ERROR_FILE_CHMOD)
		{
			switch (io_errno)
			{
				case EINTR :
				case EBADF :
				{
					is_busy_err = 1;
				}
			}
		}
		else if (ret == VCONF_ERROR_FILE_LOCK)
		{
			switch (io_errno)
			{
				case EBADF :
				case EAGAIN :
				case ENOLCK :
				{
					is_busy_err = 1;
				}
			}
		}
		else if (ret == VCONF_ERROR_FILE_WRITE)
		{
			switch (io_errno)
			{
				case 0 :
				case EAGAIN :
				case EINTR :
				case EIO :
				case ENOMEM :
				{
					is_busy_err = 1;
				}
			}
		}
		else
		{
			is_busy_err = 0;
		}

		if ((is_busy_err == 1) && (retry < VCONF_ERROR_RETRY_CNT)) {
			WARN("%s : write buf error(%d/%d). write will be retried(%d) , usleep time : %d\n",
				keynode->keyname, ret, io_errno,
				retry, (retry)*VCONF_ERROR_RETRY_SLEEP_UTIME);
			usleep((retry)*VCONF_ERROR_RETRY_SLEEP_UTIME);
			continue;
		} else {
			ERR("%s : write buf error(%d/%d). break. (%d)\n",
				keynode->keyname, ret, io_errno, retry);
			func_ret = VCONF_ERROR;
			break;
		}
	}

	if(ret == VCONF_OK) {
		g_posix_errno = VCONF_OK;
		g_vconf_errno = VCONF_OK;
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		if(func_ret == VCONF_ERROR) {
			_vconf_db_rollback_transaction();
		} else {
			_vconf_db_commit_transaction();
		}
	}
#endif

	return func_ret;
}

/*
 * This function set the value of given keys
 * @param[in] keylist the keylist which should contain changed keys
 * @return 0 on success, -1 on error
 */
API int vconf_set(keylist_t *keylist)
{
	START_TIME_CHECK

	keynode_t *got_node;

	int func_ret = VCONF_OK;
	int ret = 0;
	int prefix = 0;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is NULL");

	INFO("vconf_set (%d)START", keylist->num);

	got_node = _vconf_keylist_headnode(keylist);

	retvm_if(got_node == NULL, VCONF_ERROR, "Invalid argument: headnode is NULL");

	ret = _vconf_get_key_prefix(got_node->keyname, &prefix);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		_vconf_db_begin_transaction();
	}
#endif

	while (got_node != NULL) {
		ret = _vconf_set_key(got_node);
		if(ret != VCONF_OK) {
			func_ret = VCONF_ERROR;
		}

		got_node = _vconf_keynode_next(got_node);
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		if(func_ret == VCONF_ERROR) {
			_vconf_db_rollback_transaction();
		} else {
			_vconf_db_commit_transaction();
		}
	}
#endif

	END_TIME_CHECK

	return func_ret;
}

API int vconf_sync_key(const char *in_key)
{
	START_TIME_CHECK

	int fd;
	char path[VCONF_KEY_PATH_LEN] = {0,};
	int ret = -1;

	ret = _vconf_get_key_path(in_key, path);
	if(ret != VCONF_OK) return VCONF_ERROR;

	fd = open(path, O_RDWR);
	if(fd == -1) return VCONF_ERROR;

	fsync(fd);
	close(fd);

	END_TIME_CHECK

	return 0;
}

/*
 * This function set the integer value of given key
 * @param[in]	in_key	key
 * @param[in]	intval integer value to set
 * @return 0 on success, -1 on error
 */
API int vconf_set_int(const char *in_key, const int intval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

	int func_ret = VCONF_OK;

	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	func_ret = _vconf_keynode_set_keyname(pKeyNode, in_key);
	if(func_ret != VCONF_OK) {
		_vconf_keynode_free(pKeyNode);
		ERR("set key name error");
		return VCONF_ERROR;
	}
	_vconf_keynode_set_value_int(pKeyNode, intval);

	if (_vconf_set_key(pKeyNode) != VCONF_OK) {
		ERR("vconf_set_int(%d) : %s(%d) error", getpid(), in_key, intval);
		func_ret = VCONF_ERROR;
	} else{
		INFO("%s(%d) success", in_key, intval);
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
* This function set the boolean value of given key
* @param[in]	in_key	key
* @param[in]	boolval boolean value to set
		(Integer value 1 is 'True', and 0 is 'False')
* @return 0 on success, -1 on error
*/
API int vconf_set_bool(const char *in_key, const int boolval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

	int func_ret = VCONF_OK;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	func_ret = _vconf_keynode_set_keyname(pKeyNode, in_key);
	if(func_ret != VCONF_OK) {
		_vconf_keynode_free(pKeyNode);
		ERR("set key name error");
		return VCONF_ERROR;
	}
	_vconf_keynode_set_value_bool(pKeyNode, boolval);

	if (_vconf_set_key(pKeyNode) != VCONF_OK) {
		ERR("vconf_set_bool(%d) : %s(%d) error", getpid(), in_key, boolval);
		func_ret = VCONF_ERROR;
	} else {
		INFO("%s(%d) success", in_key, boolval);
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function set the double value of given key
 * @param[in]	in_key	key
 * @param[in]	dblval double value to set
 * @return 0 on success, -1 on error
 */
API int vconf_set_dbl(const char *in_key, const double dblval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is NULL");

	int func_ret = VCONF_OK;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	func_ret = _vconf_keynode_set_keyname(pKeyNode, in_key);
	if(func_ret != VCONF_OK) {
		_vconf_keynode_free(pKeyNode);
		ERR("set key name error");
		return VCONF_ERROR;
	}
	_vconf_keynode_set_value_dbl(pKeyNode, dblval);

	if (_vconf_set_key(pKeyNode) != VCONF_OK) {
		ERR("vconf_set_dbl(%d) : %s(%f) error", getpid(), in_key, dblval);
		func_ret = VCONF_ERROR;
	} else {
		INFO("%s(%f) success", in_key, dblval);
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function set the string value of given key
 * @param[in]	in_key	key
 * @param[in]	strval string value to set
 * @return 0 on success, -1 on error
 */
API int vconf_set_str(const char *in_key, const char *strval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is NULL");
	retvm_if(strval == NULL, VCONF_ERROR, "Invalid argument: value is NULL");

	int func_ret = VCONF_OK;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	func_ret = _vconf_keynode_set_keyname(pKeyNode, in_key);
	if(func_ret != VCONF_OK) {
		_vconf_keynode_free(pKeyNode);
		ERR("set key name error");
		return VCONF_ERROR;
	}
	_vconf_keynode_set_value_str(pKeyNode, strval);

	if (_vconf_set_key(pKeyNode) != VCONF_OK) {
		ERR("vconf_set_str(%d) : %s(%s) error", getpid(), in_key, strval);
		func_ret = VCONF_ERROR;
	} else {
		INFO("%s(%s) success", in_key, strval);
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}


#ifdef SUPPORT_ELEKTRA_VALUE_FORMAT
/* keyFileUnserialize function of ELEKTRA */
static int _vconf_get_key_elektra_format(keynode_t *keynode, FILE *fp)
{
	char version[10] = {0,};
	char type[5] = {0,};
	char comment[8] = {0,};
	char file_buf[BUF_LEN] = {0,};
	char *value = NULL;
	int value_size = 0;
	int err_no = 0;
	char err_buf[100] = { 0, };
	int func_ret = VCONF_OK;

	rewind(fp);

	if (!fgets(version, sizeof(version), fp))
	{
		if(ferror(fp)) {
			err_no = errno;
		} else {
			err_no = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_FGETS;
		goto out_return;
	}
	if (strncmp(version,"RG",2)) {
		func_ret = VCONF_ERROR_WRONG_TYPE;
		goto out_return;
	}

	if (!fgets(type, sizeof(type), fp))
	{
		if(ferror(fp)) {
			err_no = errno;
		} else {
			err_no = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_FGETS;
		goto out_return;
	}

	if (!fgets(comment, sizeof(comment), fp))
	{
		if(ferror(fp)) {
			err_no = errno;
		} else {
			err_no = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_FGETS;
		goto out_return;
	}

	while(fgets(file_buf, sizeof(file_buf), fp))
	{
		if(value) {
			value_size = value_size + strlen(file_buf);
			value = (char *) realloc(value, value_size);
			if(value == NULL) {
				func_ret = VCONF_ERROR_NO_MEM;
				break;
			}
			strncat(value, file_buf, strlen(file_buf));
		} else {
			value_size = strlen(file_buf) + 1;
			value = (char *)malloc(value_size);
			if(value == NULL) {
				func_ret = VCONF_ERROR_NO_MEM;
				break;
			}
			memset(value, 0x00, value_size);
			strncpy(value, file_buf, strlen(file_buf));
		}
	}

	if(ferror(fp)) {
		err_no = errno;
		func_ret = VCONF_ERROR_FILE_FGETS;
	} else {
		if(value) {
			switch(atoi(type))
			{
				case VCONF_TYPE_INT:
				{
					_vconf_keynode_set_value_int(keynode, atoi(value));
					break;
				}
				case VCONF_TYPE_DOUBLE:
				{
					_vconf_keynode_set_value_dbl(keynode, atof(value));
					break;
				}
				case VCONF_TYPE_BOOL:
				{
					_vconf_keynode_set_value_bool(keynode, atoi(value));
					break;
				}
				case VCONF_TYPE_STRING:
				{
					_vconf_keynode_set_value_str(keynode, value);
					break;
				}
				default :
				{
					func_ret = VCONF_ERROR_WRONG_VALUE;
				}
			}
		} else {
			if(atoi(type) == VCONF_TYPE_STRING) {
				_vconf_keynode_set_value_str(keynode, "");
			} else {
				func_ret = VCONF_ERROR_WRONG_VALUE;
			}
		}
	}

out_return :
	if(err_no != 0) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_set_key_filesys(%d/%s) step(%d) failed(%d / %s)\n", keynode->type, keynode->keyname, func_ret, err_no, err_buf);
	}

	if(value) free(value);

	return func_ret;
}
#endif

static int _vconf_get_key_filesys(keynode_t *keynode, int prefix, int* io_errno)
{
	char path[VCONF_KEY_PATH_LEN] = {0,};
	int ret = -1;
	int func_ret = VCONF_OK;
	char err_buf[100] = { 0, };
	int err_no = 0;
	int type = 0;
	FILE *fp = NULL;

#ifdef VCONF_USE_BACKUP_TRANSACTION
	char backup_path[VCONF_KEY_PATH_LEN] = {0,};
#endif

#ifdef VCONF_USE_LOCK_FILE
	char tmp[VCONF_KEY_PATH_LEN] = {0,};
	char lck[VCONF_KEY_PATH_LEN] = {0,};
	char *pCh;
	FILE *lckfp = NULL;
#endif

	errno = 0;

	ret = _vconf_get_key_path(keynode->keyname, path);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_CHECK_INITIALIZED
	if(prefix == VCONF_BACKEND_MEMORY && VCONF_NOT_INITIALIZED)
	{
		func_ret = VCONF_ERROR_NOT_INITIALIZED;
		goto out_return;
	}
#endif

#ifdef VCONF_USE_BACKUP_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		_vconf_get_backup_path(keynode->keyname, backup_path);
		ret = _vconf_backup_check(path, backup_path);
		if(ret != VCONF_OK) {
			func_ret = ret;
			err_no = errno;
			goto out_return;
		}
	}
#endif

	if( (fp = fopen(path, "r")) == NULL ) {
		func_ret = VCONF_ERROR_FILE_OPEN;
		err_no = errno;
		goto out_return;
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if (prefix != VCONF_BACKEND_DB)
#endif
	{
#ifdef VCONF_USE_LOCK_FILE
		pCh = strrchr(path, '/');
		strncat(tmp, path, pCh - path);
		snprintf(lck, VCONF_KEY_PATH_LEN, VCONF_LCK_FILE_FMT, tmp, pCh+1);
		if( (lckfp = fopen(lck, "r")) == NULL ) {
			func_ret = VCONF_ERROR_FILE_OPEN;
			err_no = errno;
			goto out_close;
		}
		ret = _vconf_set_read_lock(fileno(lckfp));
#else
		ret = _vconf_set_read_lock(fileno(fp));
#endif
		if(ret == -1) {
			func_ret = VCONF_ERROR_FILE_LOCK;
			err_no = errno;
			goto out_close;
		}
	}

	/* read data type */
	if(!fread((void*)&type, sizeof(int), 1, fp)) {
		if(ferror(fp)) {
			err_no = errno;
		} else {
			errno = EAGAIN;
		}
		func_ret = VCONF_ERROR_FILE_FREAD;
		goto out_unlock;
	}

	/* read data value */
	switch(type)
	{
		case VCONF_TYPE_INT:
		{
			int value_int = 0;
			int read_size = 0;
			read_size = fread((void*)&value_int, sizeof(int), 1, fp);
			if((read_size <= 0) || (read_size > sizeof(int))) {
				if(ferror(fp)) {
					err_no = errno;
				} else {
					errno = EAGAIN;
				}
				func_ret = VCONF_ERROR_FILE_FREAD;
				goto out_unlock;
			} else {
				_vconf_keynode_set_value_int(keynode, value_int);
			}

			break;
		}
		case VCONF_TYPE_DOUBLE:
		{
			double value_dbl = 0;
			int read_size = 0;
			read_size = fread((void*)&value_dbl, sizeof(double), 1, fp);
			if((read_size <= 0) || (read_size > sizeof(double))) {
				if(ferror(fp)) {
					err_no = errno;
				} else {
					errno = EAGAIN;
				}
				func_ret = VCONF_ERROR_FILE_FREAD;
				goto out_unlock;
			} else {
				_vconf_keynode_set_value_dbl(keynode, value_dbl);
			}

			break;
		}
		case VCONF_TYPE_BOOL:
		{
			int value_int = 0;
			int read_size = 0;
			read_size = fread((void*)&value_int, sizeof(int), 1, fp);
			if((read_size <= 0) || (read_size > sizeof(int))) {
				if(ferror(fp)) {
					err_no = errno;
				} else {
					errno = EAGAIN;
				}
				func_ret = VCONF_ERROR_FILE_FREAD;
				goto out_unlock;
			} else {
				_vconf_keynode_set_value_bool(keynode, value_int);
			}

			break;
		}
		case VCONF_TYPE_STRING:
		{
			char file_buf[BUF_LEN] = {0,};
			char *value = NULL;
			int value_size = 0;

			while(fgets(file_buf, sizeof(file_buf), fp))
			{
				if(value) {
					value_size = value_size + strlen(file_buf);
					value = (char *) realloc(value, value_size);
					if(value == NULL) {
						func_ret = VCONF_ERROR_NO_MEM;
						break;
					}
					strncat(value, file_buf, strlen(file_buf));
				} else {
					value_size = strlen(file_buf) + 1;
					value = (char *)malloc(value_size);
					if(value == NULL) {
						func_ret = VCONF_ERROR_NO_MEM;
						break;
					}
					memset(value, 0x00, value_size);
					strncpy(value, file_buf, strlen(file_buf));
				}
			}

			if(ferror(fp)) {
				err_no = errno;
				func_ret = VCONF_ERROR_FILE_FGETS;
			} else {
				if(value) {
					_vconf_keynode_set_value_str(keynode, value);
				} else {
					_vconf_keynode_set_value_str(keynode, "");
				}
			}
			if(value)
				free(value);

			break;
		}
		default :
#ifdef SUPPORT_ELEKTRA_VALUE_FORMAT
			func_ret = _vconf_get_key_elektra_format(keynode, fp);
#else
			func_ret = VCONF_ERROR_WRONG_TYPE;
#endif
	}

out_unlock :
#ifdef VCONF_USE_SQLFS_TRANSACTION
	if (prefix != VCONF_BACKEND_DB)
#endif
	{
#ifdef VCONF_USE_LOCK_FILE
		ret = _vconf_set_unlock(fileno(lckfp));
#else
		ret = _vconf_set_unlock(fileno(fp));
#endif
		if(ret == -1) {
			func_ret = VCONF_ERROR_FILE_LOCK;
			err_no = errno;
			goto out_close;
		}
	}

out_close :
	fclose(fp);
#ifdef VCONF_USE_LOCK_FILE
	if(lckfp)
		fclose(lckfp);
#endif

out_return :
	if(err_no != 0) {
		strerror_r(err_no, err_buf, 100);
		ERR("_vconf_get_key_filesys(%d-%s) step(%d) failed(%d / %s)\n", keynode->type, keynode->keyname, func_ret, err_no, err_buf);
	}

	*io_errno = err_no;

	return func_ret;
}


int _vconf_get_key(keynode_t *keynode)
{
	int func_ret = VCONF_OK;
	int ret = 0;
	int is_busy_err = 0;
	int retry = -1;
	int prefix = 0;
	int io_errno = 0;

	ret = _vconf_get_key_prefix(keynode->keyname, &prefix);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		_vconf_db_begin_transaction();
	}
#endif

	while((ret = _vconf_get_key_filesys(keynode, prefix, &io_errno)) != VCONF_OK)
	{
		is_busy_err = 0;
		retry++;

		g_posix_errno = io_errno;
		g_vconf_errno = ret;

#ifdef VCONF_CHECK_INITIALIZED
		if(VCONF_NOT_INITIALIZED)
		{
			ERR("%s : vconf is not initialized\n", keynode->keyname);
			is_busy_err = 1;
		}
		else if(ret == VCONF_ERROR_FILE_OPEN)
#else
		if(ret == VCONF_ERROR_FILE_OPEN)
#endif
		{
			switch (errno)
			{
				case EAGAIN :
				case EMFILE :
				case ETXTBSY :
				{
					is_busy_err = 1;
				}
			}
		}
		else if (ret == VCONF_ERROR_FILE_LOCK)
		{
			switch (errno)
			{
				case EBADF :
				case EACCES :
				case EAGAIN :
				case ENOLCK :
				{
					is_busy_err = 1;
				}
			}
		}
		else if (ret == VCONF_ERROR_FILE_FREAD)
		{
			switch (errno)
			{
				case EAGAIN :
				case EINTR :
				case EIO :
				{
					is_busy_err = 1;
				}
			}
		}
		else
		{
			is_busy_err = 0;
		}

		if ((is_busy_err == 1) && (retry < VCONF_ERROR_RETRY_CNT)) {
			WARN("%s : read buf error(%d / errno - %d). read will be retried(%d) , %d\n", keynode->keyname, ret, errno, retry, (retry)*VCONF_ERROR_RETRY_SLEEP_UTIME);
			usleep((retry)*VCONF_ERROR_RETRY_SLEEP_UTIME);
			continue;
		} else {
			ERR("%s : read buf error(%d). break\n",  keynode->keyname, ret);
			func_ret = VCONF_ERROR;
			break;
		}
	}

	if(ret == VCONF_OK) {
		g_posix_errno = VCONF_OK;
		g_vconf_errno = VCONF_OK;
	}

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		if(func_ret == VCONF_ERROR) {
			_vconf_db_rollback_transaction();
		} else {
			_vconf_db_commit_transaction();
		}
	}
#endif

	return func_ret;
}

/*
static int _vconf_check_value_integrity(const void *value, int type)
{
	int i = 0;

	if ((type == VCONF_TYPE_STRING) && (value != NULL)) {
		return 0;
	}

	if ((value) && (strlen(value) > 0)) {
		if ((type == VCONF_TYPE_INT) ||
			(type == VCONF_TYPE_BOOL)||
			(type == VCONF_TYPE_DOUBLE)) {
			while (*(((char *)value) + i) != '\0') {
				if ( !isdigit(*(((char *)value) + i)) ) {
					if ((type != VCONF_TYPE_BOOL) &&
						(*(((char *)value) + i) != '-')) {
						if ((type == VCONF_TYPE_DOUBLE) &&
							(*(((char *)value) + i) != '.')) {
							ERR("ERROR : vconf value is not digit.");
							return -1;
						}
					}
				}
				i++;
			}
		}

		return 0;
	} else {
		ERR("ERROR : vconf value is NULL.");
		return -2;
	}
}
*/

int _vconf_path_is_dir(char* path)
{
	struct stat entryInfo;

	if(lstat(path, &entryInfo) == 0 ) {
		if( S_ISDIR( entryInfo.st_mode ) ) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return VCONF_ERROR;
	}
}

API int vconf_get(keylist_t *keylist, const char *dirpath, get_option_t option)
{
	DIR *dir = NULL;
	struct dirent entry;
	struct dirent *result = NULL;
	char full_file_path[VCONF_KEY_PATH_LEN] = {0,};
	char file_path[VCONF_KEY_PATH_LEN] = {0,};
	char full_path[VCONF_KEY_PATH_LEN] = {0,};
	char err_buf[ERR_LEN] = {0,};
	int rc = 0;
	int func_ret = 0;
	int ret = 0;
	int is_dir = 0;
	int prefix = 0;

	keynode_t *temp_keynode;

	retvm_if(keylist == NULL, VCONF_ERROR, "Invalid argument: keylist is null");
	retvm_if(dirpath == NULL, VCONF_ERROR, "Invalid argument: dirpath is null");

	temp_keynode = _vconf_keylist_headnode(keylist);

	if ((NULL != temp_keynode) && (VCONF_GET_KEY != option)) {
		ERR("Not support mode : Only VCONF_GET_KEY \
		option support To retrieve key with keylist");
		return VCONF_ERROR;
	}

	if(temp_keynode != NULL) {
		while(_vconf_keynode_next(temp_keynode)) {
			temp_keynode = _vconf_keynode_next(temp_keynode);
		}
	}

	ret = _vconf_get_key_path(dirpath, full_path);
	retvm_if(ret != VCONF_OK, ret, "Invalid argument: key is not valid");


	ret = _vconf_get_key_prefix(dirpath, &prefix);
	retv_if(ret != VCONF_OK, ret);

#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		_vconf_db_begin_transaction();
	}
#endif

	is_dir = _vconf_path_is_dir(full_path);
	if(is_dir == 1) {
		if((dir=opendir(full_path)) == NULL) {
			strerror_r(errno, err_buf, ERR_LEN);
			ERR("ERROR : open directory(%s) fail(%s)", dirpath, err_buf);
			func_ret = VCONF_ERROR;
			goto out_unlock;
		}

		if((readdir_r(dir, &entry, &result)) != 0) {
			strerror_r(errno, err_buf, ERR_LEN);
			ERR("ERROR : read directory(%s) fail(%s)", dirpath, err_buf);
			func_ret = VCONF_ERROR;
		}

		while(result != NULL)
		{
			if(( strcmp( entry.d_name, ".") == 0 ) || ( strcmp( entry.d_name, "..") == 0 )) {
		            goto NEXT;
			}

			keynode_t* keynode = _vconf_keynode_new();
			if(keynode == NULL) {
				closedir(dir);
				ERR("Invalid argument: key malloc fail");
				func_ret = VCONF_ERROR;
				goto out_unlock;
			}
#ifdef COMBINE_FOLDER
			if('/' == *(dirpath+(strlen(dirpath)-1))) {
				snprintf(file_path, VCONF_KEY_PATH_LEN, "%s%s", dirpath, entry.d_name);
				snprintf(full_file_path, VCONF_KEY_PATH_LEN, "%s%s", full_path, entry.d_name);
			} else {
				snprintf(file_path, VCONF_KEY_PATH_LEN, "%s/%s", dirpath, entry.d_name);
				snprintf(full_file_path, VCONF_KEY_PATH_LEN, "%s/%s", full_path, entry.d_name);
			}
#else
			snprintf(file_path, VCONF_KEY_PATH_LEN, "%s/%s", dirpath, entry.d_name);
			snprintf(full_file_path, VCONF_KEY_PATH_LEN, "%s/%s", full_path, entry.d_name);
#endif
			rc = _vconf_path_is_dir(full_file_path);
			if(rc != VCONF_ERROR) {
				if(rc == 1) {
					/* directory */
					if(option == VCONF_GET_KEY) {
						_vconf_keynode_free(keynode);
						goto NEXT;
					} else {
						_vconf_keynode_set_keyname(keynode, file_path);
						_vconf_keynode_set_dir(keynode);
					}
				} else {
					_vconf_keynode_set_keyname(keynode, file_path);
					_vconf_get_key(keynode);
				}

				if (keylist->head && temp_keynode != NULL)
				{
					temp_keynode->next = keynode;
					temp_keynode = _vconf_keynode_next(temp_keynode);
				}
				else {
					keylist->head = keynode;
					temp_keynode = keylist->head;
				}
				keylist->num += 1;
			} else {
				_vconf_keynode_free(keynode);

				memset(err_buf, 0x00, sizeof(err_buf));
				strerror_r(errno, err_buf, sizeof(err_buf));
				ERR("ERROR : get path(%s) fail(%s)", file_path, err_buf);
				func_ret = VCONF_ERROR;
			}

	NEXT:
			if((readdir_r(dir, &entry, &result)) != 0) {
				memset(err_buf, 0x00, sizeof(err_buf));
				strerror_r(errno, err_buf, sizeof(err_buf));
				ERR("ERROR : read directory(%s) fail(%s)", dirpath, err_buf);
				func_ret = VCONF_ERROR;
			}
		}

		if((closedir(dir)) != 0) {
			memset(err_buf, 0x00, sizeof(err_buf));
			strerror_r(errno, err_buf, sizeof(err_buf));
			ERR("ERROR : close directory(%s) fail(%s)", dirpath, err_buf);
			func_ret = VCONF_ERROR;
		}
	} else if(is_dir == 0) {
		keynode_t* keynode = _vconf_keynode_new();
		retvm_if(keynode == NULL, VCONF_ERROR, "Invalid argument: key malloc fail");

		_vconf_keynode_set_keyname(keynode, dirpath);

		_vconf_get_key(keynode);

		if (keylist->head && temp_keynode != NULL) {
			temp_keynode->next = keynode;
			//temp_keynode = _vconf_keynode_next(temp_keynode);
		} else {
			keylist->head = keynode;
			temp_keynode = keylist->head;
		}
		keylist->num += 1;
	} else {
		func_ret = VCONF_ERROR;
		goto out_unlock;
	}
	vconf_keylist_rewind(keylist);

out_unlock:
#ifdef VCONF_USE_SQLFS_TRANSACTION
	if(prefix == VCONF_BACKEND_DB) {
		if(func_ret == VCONF_ERROR) {
			_vconf_db_rollback_transaction();
		} else {
			_vconf_db_commit_transaction();
		}
	}
#endif

	return func_ret;
}

/*
 * This function get the integer value of given key
 * @param[in]	in_key	key
 * @param[out]	intval output buffer
 * @return 0 on success, -1 on error
 */
API int vconf_get_int(const char *in_key, int *intval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");
	retvm_if(intval == NULL, VCONF_ERROR, "Invalid argument: output buffer is null");

	int func_ret = VCONF_ERROR;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	_vconf_keynode_set_keyname(pKeyNode, in_key);

	if (_vconf_get_key(pKeyNode) != VCONF_OK)
		ERR("vconf_get_int(%d) : %s error", getpid(), in_key);
	else {
		*intval = pKeyNode->value.i;
		if(pKeyNode->type == VCONF_TYPE_INT) {
			INFO("%s(%d) success", in_key, *intval);
			func_ret = VCONF_OK;
		} else {
			ERR("The type(%d) of keynode(%s) is not INT", pKeyNode->type, pKeyNode->keyname);
		}
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function get the boolean value of given key
 * @param[in]	in_key	key
 * @param[out]	boolval output buffer
 * @return 0 on success, -1 on error
 */
API int vconf_get_bool(const char *in_key, int *boolval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");
	retvm_if(boolval == NULL, VCONF_ERROR, "Invalid argument: output buffer is null");

	int func_ret = VCONF_ERROR;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	_vconf_keynode_set_keyname(pKeyNode, in_key);

	if (_vconf_get_key(pKeyNode) != VCONF_OK)
		ERR("vconf_get_bool(%d) : %s error", getpid(), in_key);
	else {
		*boolval = !!(pKeyNode->value.b);
		if(pKeyNode->type == VCONF_TYPE_BOOL) {
			INFO("%s(%d) success", in_key, *boolval);
			func_ret = VCONF_OK;
		} else {
			ERR("The type(%d) of keynode(%s) is not BOOL", pKeyNode->type, pKeyNode->keyname);
		}
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function get the double value of given key
 * @param[in]	in_key	key
 * @param[out]	dblval output buffer
 * @return 0 on success, -1 on error
 */
API int vconf_get_dbl(const char *in_key, double *dblval)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");
	retvm_if(dblval == NULL, VCONF_ERROR, "Invalid argument: output buffer is null");

	int func_ret = VCONF_ERROR;
	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, VCONF_ERROR, "key malloc fail");

	_vconf_keynode_set_keyname(pKeyNode, in_key);

	if (_vconf_get_key(pKeyNode) != VCONF_OK)
		ERR("vconf_get_dbl(%d) : %s error", getpid(), in_key);
	else {
		*dblval = pKeyNode->value.d;

		if(pKeyNode->type == VCONF_TYPE_DOUBLE) {
			INFO("%s(%f) success", in_key, *dblval);
			func_ret = VCONF_OK;
		} else
			ERR("The type(%d) of keynode(%s) is not DBL", pKeyNode->type, pKeyNode->keyname);
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function get the string value of given key
 * @param[in]	in_key	key
 * @return pointer of key value on success, NULL on error
 */
API char *vconf_get_str(const char *in_key)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, NULL, "Invalid argument: key is null");

	keynode_t* pKeyNode = _vconf_keynode_new();
	retvm_if(pKeyNode == NULL, NULL, "key malloc fail");

	_vconf_keynode_set_keyname(pKeyNode, in_key);

	char *strval = NULL;
	char *tempstr = NULL;

	if (_vconf_get_key(pKeyNode) != VCONF_OK) {
		ERR("vconf_get_str(%d) : %s error", getpid(), in_key);
	} else {
		if(pKeyNode->type == VCONF_TYPE_STRING)
			tempstr = pKeyNode->value.s;
		else
			//FATAL("The type(%d) of keynode(%s) is not STR", pKeyNode->type, pKeyNode->keyname);
			ERR("The type(%d) of keynode(%s) is not STR", pKeyNode->type, pKeyNode->keyname);

		if(tempstr) {
			strval = strdup(tempstr);
			INFO("%s(%s) success", in_key, strval);
		}
	}

	_vconf_keynode_free(pKeyNode);

	END_TIME_CHECK

	return strval;
}

/*
 * This function unset given key
 * @param[in]	in_key	key
 * @return 0 on success, -1 on error
 */
API int vconf_unset(const char *in_key)
{
	START_TIME_CHECK

	char path[VCONF_KEY_PATH_LEN] = {0,};
	int ret = -1;
	int err_retry = VCONF_ERROR_RETRY_CNT;
	int func_ret = VCONF_OK;

	WARN("vconf_unset: %s. THIS API(vconf_unset) WILL BE DEPRECATED", in_key);

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");

	ret = _vconf_get_key_path(in_key, path);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "Invalid argument: key is not valid");

	retvm_if(access(path, F_OK) == -1, VCONF_ERROR, "Error : key(%s) is not exist", in_key);

	do {
		ret = remove(path);
		if(ret == -1) {
			ERR("vconf_unset error(%d) : %s", errno, in_key);
			func_ret = VCONF_ERROR;
		} else {
			func_ret = VCONF_OK;
			break;
		}
	} while(err_retry--);

	END_TIME_CHECK

	return func_ret;
}

/*
 * This function unset given key recursively
 * @param[in]	in_dir	Directory name for removing
 * @return 0 on success, -1 on error
 */
API int vconf_unset_recursive(const char *in_dir)
{
	START_TIME_CHECK

	DIR *dir = NULL;
	struct dirent entry;
	struct dirent *result = NULL;
	char fullpath[VCONF_KEY_PATH_LEN] = {0,};
	char dirpath[VCONF_KEY_PATH_LEN] = {0,};
	char err_buf[ERR_LEN] = {0,};
	int rc = 0;
	int func_ret = 0;
	int ret = 0;

	WARN("vconf_unset_recursive: %s. THIS API(vconf_unset_recursive) WILL BE DEPRECATED", in_dir);

	retvm_if(in_dir == NULL, VCONF_ERROR, "Invalid argument: dir path is null");

#ifdef COMBINE_FOLDER
	if (strncmp(in_dir, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1) == 0) {
		snprintf(dirpath, VCONF_KEY_PATH_LEN, "%sdb", BACKEND_SYSTEM_DIR);
	} else if (0 == strncmp(in_dir, BACKEND_FILE_PREFIX, sizeof(BACKEND_FILE_PREFIX) - 1)) {
		snprintf(dirpath, VCONF_KEY_PATH_LEN, "%sfile", BACKEND_SYSTEM_DIR);
	} else if (0 == strncmp(in_dir, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
		snprintf(dirpath, VCONF_KEY_PATH_LEN, "%smemory", BACKEND_MEMORY_DIR);
	} else {
		ERR("Invalid argument: wrong prefix of key(%s)", in_dir);
		return VCONF_ERROR;
	}

	char key_name[VCONF_KEY_PATH_LEN] = {0,};
	const char* split_key = strchr((const char*)in_dir, (int)'/');
	snprintf(key_name, VCONF_KEY_PATH_LEN, "%s", split_key+1);

	if(key_name[strlen(key_name)-1] == '/') {
		key_name[strlen(key_name)-1] = '+';
	} else {
		key_name[strlen(key_name)] = '+';
	}
#else
	ret = _vconf_get_key_path(in_dir, dirpath);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "Invalid argument: key is not valid");
#endif

	if((dir=opendir(dirpath)) == NULL) {
		strerror_r(errno, err_buf, ERR_LEN);
		ERR("ERROR : open directory(%s) fail(%s)", in_dir, err_buf);
		return VCONF_ERROR;
	}

	if((readdir_r(dir, &entry, &result)) != 0) {
		strerror_r(errno, err_buf, ERR_LEN);
		ERR("ERROR : read directory(%s) fail(%s)", in_dir, err_buf);
		func_ret = VCONF_ERROR;
	}

	while(result != NULL)
	{
		if(( strcmp( entry.d_name, ".") == 0 ) || ( strcmp( entry.d_name, "..") == 0 )) {
	            goto NEXT;
		}

#ifdef COMBINE_FOLDER
		if(!strstr(entry.d_name, key_name)) {
				goto NEXT;
		}
#endif

		snprintf(fullpath,VCONF_KEY_PATH_LEN, "%s/%s", dirpath, entry.d_name);

		ret = _vconf_path_is_dir(fullpath);
		if(ret != VCONF_ERROR) {
			if(ret == 1) {
				rc = vconf_unset_recursive(fullpath);
				if(rc == VCONF_ERROR)
					func_ret = VCONF_ERROR;
			}

			rc = remove(fullpath);
			if(rc == -1) {
				memset(err_buf, 0x00, sizeof(err_buf));
				strerror_r(errno, err_buf, sizeof(err_buf));
				ERR("ERROR : remove path(%s) fail(%s)", in_dir, err_buf);
				func_ret = VCONF_ERROR;
			}
		} else {
			memset(err_buf, 0x00, sizeof(err_buf));
			strerror_r(errno, err_buf, sizeof(err_buf));
			ERR("ERROR : remove path(%s) fail(%s)", in_dir, err_buf);
			func_ret = VCONF_ERROR;
		}
NEXT:
		if((readdir_r(dir, &entry, &result)) != 0) {
			memset(err_buf, 0x00, sizeof(err_buf));
			strerror_r(errno, err_buf, sizeof(err_buf));
			ERR("ERROR : read directory(%s) fail(%s)", in_dir, err_buf);
			func_ret = VCONF_ERROR;
		}
	}

	if((closedir(dir)) != 0) {
		memset(err_buf, 0x00, sizeof(err_buf));
		strerror_r(errno, err_buf, sizeof(err_buf));
		ERR("ERROR : close directory(%s) fail(%s)", in_dir, err_buf);
		func_ret = VCONF_ERROR;
	}
#if 0
	if(func_ret == VCONF_OK) {
		if((remove(in_dir)) == -1) {
			memset(err_buf, 0x00, sizeof(err_buf));
			strerror_r(errno, err_buf, sizeof(err_buf));
			ERR("ERROR : remove path(%s) fail(%s)", in_dir, err_buf);
			func_ret = VCONF_ERROR;
		}
	}
#endif

	return func_ret;
}

API int vconf_notify_key_changed(const char *in_key, vconf_callback_fn cb, void *user_data)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");
	retvm_if(cb == NULL, VCONF_ERROR, "Invalid argument: cb(%p)", cb);

	if (_vconf_kdb_add_notify(in_key, cb, user_data)) {
		if(errno != EALREADY) {
			ERR("vconf_notify_key_changed : key(%s) add notify fail", in_key);
		}
		return VCONF_ERROR;
	}

	INFO("%s noti is added", in_key);

	END_TIME_CHECK

	return VCONF_OK;
}

API int vconf_ignore_key_changed(const char *in_key, vconf_callback_fn cb)
{
	START_TIME_CHECK

	retvm_if(in_key == NULL, VCONF_ERROR, "Invalid argument: key is null");
	retvm_if(cb == NULL, VCONF_ERROR, "Invalid argument: cb(%p)", cb);

	if (_vconf_kdb_del_notify(in_key, cb)) {
		if(errno != ENOENT) {
			ERR("vconf_ignore_key_changed() failed: key(%s)", in_key);
		}
		return VCONF_ERROR;
	}

	INFO("%s noti removed", in_key);

	END_TIME_CHECK

	return VCONF_OK;
}

API int vconf_get_ext_errno(void)
{
	int ret = VCONF_OK;

	if(g_vconf_errno != VCONF_OK) {
		switch(g_posix_errno) {
			case ENOENT:
				ret = VCONF_ERROR_FILE_NO_ENT;
				break;
			case ENOMEM:
			case ENOSPC:
				ret = VCONF_ERROR_FILE_NO_MEM;
				break;
			case EAGAIN:
			case ETIMEDOUT:
			case EBUSY:
			case EMFILE:
				ret = VCONF_ERROR_FILE_BUSY;
				break;
			case EACCES:
			case EPERM:
				ret = VCONF_ERROR_FILE_PERM;
				break;
			default:
				ret = g_vconf_errno;
		}
	}

	INFO("posix errno : %d / vconf errno : %d", g_posix_errno, g_vconf_errno);

	return ret;
}



