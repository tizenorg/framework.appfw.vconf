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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <vconf.h>
#include <glib-object.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/smack.h>
#include <fcntl.h>
#include <errno.h>

#include <wordexp.h>

#define VCONF_LOG 3

#include "vconf-log.h"

#include "vconf-internals.h"

enum {
	VCONFTOOL_TYPE_NO = 0x00,
	VCONFTOOL_TYPE_STRING,
	VCONFTOOL_TYPE_INT,
	VCONFTOOL_TYPE_DOUBLE,
	VCONFTOOL_TYPE_BOOL
};

#define VCONF_RESTORE_ALL "all"

#define BUFSIZE		1024
#define CMDSIZE		4096

#define VCONF_TMP_LIST "/opt/var/kdb/.vconf_tmp_list"
#define VCONF_ERROR_RETRY_CNT 7

#define VCONF_SMACK_WHITELIST_PATH "/usr/share/smack-label/smack-label-vconfkey-whitelist.txt"
#define VCONF_INVALID_SMACK_LABEL "invalid_label::Must_use_predefined_smack_label"

const char *DB_PREFIX = "/opt/var/kdb/";
const char *FILE_PREFIX = "/opt/var/kdb/";
const char *MEMORY_PREFIX = "/var/run/";
const char *MEMORY_INIT = "/opt/var/kdb/memory_init/";

static char *guid = NULL;
static char *uid = NULL;
static char *smack_label = NULL;
static char *vconf_type = NULL;
static int is_recursive = FALSE;
static int is_initialization = FALSE;
static int is_forced = FALSE;
static int get_num = 0;

static GOptionEntry entries[] = {
	{"type", 't', 0, G_OPTION_ARG_STRING, &vconf_type, "type of value",
	 "int|bool|double|string"},
	{"recursive", 'r', 0, G_OPTION_ARG_NONE, &is_recursive,
	 "retrieve keys recursively", NULL},
	{"guid", 'g', 0, G_OPTION_ARG_STRING, &guid, "group permission", NULL},
	{"uid", 'u', 0, G_OPTION_ARG_STRING, &uid, "user permission", NULL},
	{"initialization", 'i', 0, G_OPTION_ARG_NONE, &is_initialization,
	 "memory backend initialization", NULL},
	{"force", 'f', 0, G_OPTION_ARG_NONE, &is_forced,
	 "overwrite vconf values by force", NULL},
	{"smack", 's', 0, G_OPTION_ARG_STRING, &smack_label,
	 "smack access label", NULL},
	{NULL}
};

static void get_operation(char *input);
static void recursive_get(char *subDIR, int level);
static void print_keylist(keylist_t *keylist, keynode_t *temp_node, int level);

static void print_help(const char *cmd)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "[Set vconf value]\n");
	fprintf(stderr,
		"       %s set -t <TYPE> <KEY NAME> <VALUE> <OPTIONS>\n", cmd);
	fprintf(stderr, "                 <TYPE>=int|bool|double|string\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"       Ex) %s set -t string db/testapp/key1 \"This is test\" \n",
		cmd);
	fprintf(stderr, "\n");
#ifdef VCONF_ENABLE_DAC_CONTROL
	fprintf(stderr, "       <OPTIONS>\n");
	fprintf(stderr,
		"          -g <GUID> : Set Effective group id. The key's permission will be set to 0664.\n");
	fprintf(stderr,
		"           Ex) %s set -t string db/testapp/key1 \"This is test\" -g 425\n",
		cmd);
	fprintf(stderr,
		"           NOTE: -g and -u cannot be userd together. -u ignores -g option.\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"          -u <UID> : Set Effective user id. The key's permission will be set to 0644.\n");
	fprintf(stderr,
		"           Ex) %s set -t string db/testapp/key1 \"This is test\" -u 5000\n",
		cmd);
	fprintf(stderr,
		"           NOTE: -g and -u cannot be userd together. -u ignores -g option.\n");
	fprintf(stderr, "\n");
#endif
	fprintf(stderr,
		"          -i : Install memory backend key into flash space for backup.\n");
	fprintf(stderr,
		"           Ex) %s set -t string memory/testapp/key1 \"This is test\" -i\n",
		cmd);
	fprintf(stderr, "\n");
	fprintf(stderr,
		"          -f : Overwrite values by force, even when vconf values are already exist.\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"          -s <SMACK LABEL>: Set smack access label for the vconf key.\n");
#ifdef VCONF_SMACK_WHITELIST
	fprintf(stderr,
		"           Ex) %s set -t string db/testapp/key1 \"This is test\" -s tizen::vconf::public::r\n",
		cmd);
	fprintf(stderr,
		"           NOTE: If -s option is not used, smack access label will be set as unavailable smack label.\n");
#else
	fprintf(stderr,
		"           Ex) %s set -t string db/testapp/key1 \"This is test\" -s system::vconf_setting\n",
		cmd);
	fprintf(stderr,
		"           NOTE: If -s option is not used, the default smack access label system::vconf will be set.\n");
#endif
	fprintf(stderr,
		"           NOTE: Maximum label size is 255 byte and '/', '~', ' ' is not permitted.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "[Get vconf value]\n");
	fprintf(stderr, "       %s get <OPTIONS> <KEY NAME>\n", cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "       <OPTIONS>\n");
	//fprintf(stderr, "          -r : retrieve all keys included in sub-directorys \n");
	//fprintf(stderr, "       Ex) %s get db/testapp/key1\n", cmd);
	//fprintf(stderr, "           %s get db/testapp/\n", cmd);
	//fprintf(stderr, "\n");
	fprintf(stderr, "[Unset vconf value]\n");
	fprintf(stderr, "       %s unset <KEY NAME>\n", cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "       Ex) %s unset db/testapp/key1\n", cmd);
	fprintf(stderr, "\n");
}

static int check_type(void)
{
	if (vconf_type) {
		if (!strncasecmp(vconf_type, "int", 3))
			return VCONFTOOL_TYPE_INT;
		else if (!strncasecmp(vconf_type, "string", 6))
			return VCONFTOOL_TYPE_STRING;
		else if (!strncasecmp(vconf_type, "double", 6))
			return VCONFTOOL_TYPE_DOUBLE;
		else if (!strncasecmp(vconf_type, "bool", 4))
			return VCONFTOOL_TYPE_BOOL;
	}
	return VCONFTOOL_TYPE_NO;
}

static int __system(char * cmd)
{
	int status;
	pid_t cpid;

	if((cpid = fork()) < 0) {
		perror("fork");
		return -1;
	}

	if (cpid == 0) {
		/* child */
		wordexp_t p;
		char **w;

		wordexp(cmd, &p, 0);
		w = p.we_wordv;

		execv(w[0], (char *const *)w);

		wordfree(&p);

		_exit(-1);
	} else {
		/* parent */
		if (waitpid(cpid, &status, 0) == -1) {
			perror("waitpid failed\n");
			return -1;
		}
		if (WIFSIGNALED(status)) {
			printf("signal(%d)\n", WTERMSIG(status));
			perror("exit by signal\n");
			return -1;
		}
		if (!WIFEXITED(status)) {
			perror("exit abnormally\n");
			return -1;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status)) {
			perror("child return error\n");
			return -1;
		}
	}

	return 0;
}

static void disable_invalid_char(char* src)
{
	char* tmp;

	for(tmp = src; *tmp; ++tmp)
	{
		if( (*tmp == ';') || (*tmp == '|') )
		{
			fprintf(stderr,"[vconf] invalid char is found\n");
			*tmp = '_';
		}
	}
}

#ifdef VCONF_SMACK_WHITELIST
static int check_smack_whitelist(const char *label)
{
	FILE *fp = NULL;
	char file_buf[BUFSIZE] = {0,};
	int ret = -1;

	if(!label) return -1;

	if( (fp = fopen(VCONF_SMACK_WHITELIST_PATH, "r")) == NULL ) {
		switch(errno)
		{
			case EFAULT :
			case ENOENT :
			{
				/* if smack white list file does not exist, smack whitelist feature is disabled */
				ret = 0;
				break;
			}
			default :
			{
				fprintf(stderr,
				"[vconf] smack whitelist file open fail\n");
			}
		}

		goto func_out;
	}

	while(fgets(file_buf, sizeof(file_buf), fp))
	{
		if(strncmp(file_buf,label,strlen(label)) == 0) {
			ret = 0;
			break;
		}
	}

func_out:
	if (fp)
		fclose(fp);

	return ret;
}
#endif

static void _add_vconf_key_to_list(const char *key, const char *list)
{
	FILE *fp = NULL;
	int ret = 0;

	if((!key) || (strlen(key) == 0)) return;

	if( (fp = fopen(list, "a+")) == NULL ) {
		fprintf(stderr, "[vconf] failed(%d) to open key(%s),list(%s)\n", errno, key, list);
		return;
	}

	ret = fprintf(fp,"%s\n",key);
	if(ret < strlen(key)) {
		fprintf(stderr, "[vconf] failed(%d) to add key(%s),list(%s)\n", errno, key, list);
		goto func_out;
	}

	fflush(fp);

	ret = fdatasync(fileno(fp));
	if(ret != 0) {
		fprintf(stderr, "[vconf] failed(%d) to sync key(%s)\n",errno, key);
	}
func_out :
	if(fp)
		fclose(fp);

	return;
}

static void _del_vconf_key_to_list(const char *key, const char *list)
{
	FILE *fp, *tmpfp = NULL;
	int ret = 0;
	char file_buf[BUFSIZE] = {0,};
	int func_ret = 0;

	if((!key) || (strlen(key) == 0)) return;

	if( (fp = fopen(list, "r+")) == NULL ) {
		fprintf(stderr, "[vconf] failed(%d) to open key(%s),list(%s)\n", errno, key, list);
		return;
	}

	if( (tmpfp = fopen(VCONF_TMP_LIST, "w+")) == NULL ) {
		fprintf(stderr, "[vconf] failed(%d) to open tmp file key(%s),list(%s)\n", errno, key, list);
		func_ret = -1;
		goto func_out;
	}

	while(fgets(file_buf, sizeof(file_buf), fp))
	{
		if(strncmp(file_buf,key,strlen(key)) != 0) {
			ret = fprintf(tmpfp,"%s",file_buf);
			if(ret < strlen(file_buf)) {
				fprintf(stderr, "[vconf] failed(%d) to add key(%s),list(%s) to tmp file\n", errno, key, list);
				func_ret = -1;
				goto func_out;
			}
		}
	}

	fflush(tmpfp);
	ret = fdatasync(fileno(tmpfp));
	if(ret != 0) {
		fprintf(stderr, "[vconf] failed(%d) to sync tmp file / key(%s)\n", errno, key);
		func_ret = -1;
		goto func_out;
	}


func_out :
	if(tmpfp)
		fclose(tmpfp);

	if(fp)
		fclose(fp);

	if(func_ret == 0) {
		ret = rename(VCONF_TMP_LIST,list);
		if(ret != 0) {
			fprintf(stderr, "[vconf] list(%s) rename error(%d)", list, errno);
		}
	}

	return;
}

static int _check_parent_dir(const char* path)
{
	int stat_ret = 0;
	struct stat stat_info;
	char* parent;
	char path_buf[BUFSIZE] = {0,};
	int ret = 0;

	const char* dir_smack_label = "_";
	mode_t dir_mode = 0755;

	parent = strrchr(path, '/');
	strncpy(path_buf, path, parent-path);
	path_buf[parent-path]=0;

	stat_ret = stat(path_buf,&stat_info);
	if(stat_ret){
		if(mkdir(path_buf, dir_mode) != 0) {
			if(errno == ENOENT) {
				ret = _check_parent_dir((const char*)path_buf);
				if(ret != VCONF_OK) return ret;
				if(mkdir(path_buf, dir_mode) != 0) {
					fprintf(stderr, "[vconf] mkdir(%s) error(%d)", path_buf, errno);
					return -1;
				}
			} else {
				fprintf(stderr, "[vconf] mkdir(%s) error(%d)", path_buf, errno);
				return -1;
			}
		}

		if (smack_lsetlabel(path_buf, dir_smack_label, SMACK_LABEL_ACCESS) < 0) {
			if (errno != EOPNOTSUPP) {
				fprintf(stderr, "[vconf][smack] smack set label (%s) failed (%s)\n", dir_smack_label, path_buf);
			}
		}

		if (smack_lsetlabel(path_buf, "1", SMACK_LABEL_TRANSMUTE) < 0) {
			if (errno != EOPNOTSUPP) {
				fprintf(stderr, "[vconf][smack] smack set label (%s) failed (%s)\n", dir_smack_label, path_buf);
			}
		}
	}

	return 0;
}

static int check_file_path_mode(char* key, char* file_path)
{
	int create_file = 0;
	int fd;
#ifdef VCONF_SMACK_WHITELIST
	char *setlabel = VCONF_INVALID_SMACK_LABEL;
#else
	char *setlabel = NULL;
#endif

#ifdef VCONF_ENABLE_DAC_CONTROL
	int open_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
#else
	int open_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
#endif

	if(!key) {
		fprintf(stderr,
				"[vconf] key is null\n");
		return -1;
	}

	if(!file_path) {
		fprintf(stderr,
				"[vconf] file_path is null\n");
		return -1;
	}

#ifdef VCONF_ENABLE_DAC_CONTROL
	int set_id = 0;
	if (guid || uid) {
		if (getuid() != 0) {
			fprintf(stderr,
				"[vconf] Only root user can use '-g or -u' option\n");
			return -1;
		}

		set_id = 1;
	}
#endif

	/* Check file path */
	if (access(file_path, F_OK) != 0) {
		/* fprintf(stderr,"key does not exist\n"); */

		/* Check directory & create it */
		if(_check_parent_dir(file_path)) {
			fprintf(stderr, "[vconf] fail to mkdir (%s) \n", file_path);
			return -1;
		}

		create_file = 1;
	} else if (!is_forced) {
		fprintf(stderr, "[vconf] Key already exist. Use -f option to force update\n");
		return -1;
	}

	if(create_file) {
		/* Create file */
		mode_t temp;
		temp = umask(0000);
		fd = open(file_path, O_RDWR|O_CREAT, open_mode);
		umask(temp);
		if(fd == -1) {
			fprintf(stderr, "[vconf] open(rdwr,create) error\n");
			return -1;
		}
		close(fd);

		if (smack_label) {
			setlabel = smack_label;
#ifdef VCONF_SMACK_WHITELIST
			if (check_smack_whitelist(smack_label) < 0) {
				setlabel = VCONF_INVALID_SMACK_LABEL;
			}
#endif
		}
		if(setlabel) {
#ifdef VCONF_SMACK_WHITELIST
			if(strncmp(setlabel, VCONF_INVALID_SMACK_LABEL, strlen(VCONF_INVALID_SMACK_LABEL)) == 0) {
				fprintf(stderr,
					"[vconf][smack] smack label(%s) for vconf key(%s) is not invalid\n",
					smack_label, key);
			}
#endif
			if (smack_setlabel(file_path, setlabel, SMACK_LABEL_ACCESS) < 0) {
				if (errno != EOPNOTSUPP) {
					fprintf(stderr, "[vconf][smack] smack set label (%s) failed (%s)\n", setlabel, file_path);
					return -1;
				}
			}
		}

		/* add key to installed_vconf_list */
		_add_vconf_key_to_list(key, VCONF_INSTALL_LIST);
	}

#ifdef VCONF_ENABLE_DAC_CONTROL
	if(set_id) {
		if((fd = open(file_path, O_RDONLY)) == -1) {
			fprintf(stderr, "[vconf] open(rdonly) error\n");
			return -1;
		}

		if(uid) {
			if (fchown(fd, atoi(uid), atoi(uid)) == -1) {
				fprintf(stderr, "[vconf] Fail to fchown(%d)\n", errno);
				close(fd);
				return -1;
			}
		} else if (guid) {
			if (-1 == fchown(fd, 0, atoi(guid))) {
				fprintf(stderr, "[vconf] Fail to fchown()\n");
				close(fd);
				return -1;
			}
		}

		close(fd);
	}
#endif

	return 0;
}

static char* make_file_name(char *keyname)
{
	const char *key = NULL;

	if(!keyname) {
		ERR("keyname is null");
		return NULL;
	}

#ifdef COMBINE_FOLDER
	char convert_key[BUFSIZE] = {0,};
	char *chrptr = NULL;

	snprintf(convert_key, BUFSIZE, "%s", keyname);

	chrptr = strchr((const char*)convert_key, (int)'/');
	if(!chrptr) {
		ERR("wrong key path!");
		return NULL;
	}
	chrptr = strchr((const char*)chrptr+1, (int)'/');
	while(chrptr) {
		convert_key[chrptr-convert_key] = VCONF_CONVERSION_CHAR;
		chrptr = strchr((const char*)chrptr+1, (int)'/');
	}
	key = (const char*)convert_key;
#else
	key = keyname;
#endif

	return strdup(key);
}

static int make_file_path(char *keyname, char *path)
{
	char *key = NULL;

	key = make_file_name(keyname);
	if (!key) {
		ERR("key is null");
		return -1;
	}

	if (strncmp(key, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1) == 0) {
		snprintf(path, BUFSIZE, "%s%s", DB_PREFIX, key);
	} else if (0 == strncmp(key, BACKEND_FILE_PREFIX, sizeof(BACKEND_FILE_PREFIX) - 1)) {
		snprintf(path, BUFSIZE, "%s%s", FILE_PREFIX, key);
	} else if (0 == strncmp(key, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
		snprintf(path, BUFSIZE, "%s%s", MEMORY_PREFIX, key);
	} else {
		ERR("Invalid argument: wrong prefix of key(%s)", key);
	}

	if(key)
		free(key);

	return 0;
}

static int _vconf_remove_file(const char *path)
{
	int ret = -1;
	int err_retry = VCONF_ERROR_RETRY_CNT;

	retvm_if(path == NULL, VCONF_ERROR, "[vconf] Invalid argument: key is null");
	retvm_if(access(path, F_OK) == -1, VCONF_ERROR, "[vconf] Error : key(%s) is not exist", path);

	do {
		ret = remove(path);
		if(ret == -1) {
			fprintf(stderr, "[vconf] vconf_unset error(%d) : %s", errno, path);
		} else {
			break;
		}
	} while(err_retry--);

	return ret;
}

static int _vconf_unset(const char *in_key)
{
	char path[BUFSIZE] = {0,};
	char initpath[BUFSIZE] = {0,};
	int ret = -1;
	char *key = NULL;

	retvm_if(in_key == NULL, VCONF_ERROR, "[vconf] Invalid argument: key is null");
	ret = make_file_path((char*)in_key, path);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "[vconf] Invalid argument: key(%s) is not valid", in_key);
	retvm_if(access(path, F_OK) == -1, VCONF_ERROR, "[vconf] Error : key(%s) is not exist", in_key);

	ret = _vconf_remove_file(path);
	if(ret == VCONF_OK) {
		/* delete key from installed_vconf_list */
		_del_vconf_key_to_list(in_key, VCONF_INSTALL_LIST);
	}

	if(strncmp(in_key, BACKEND_MEMORY_PREFIX, strlen(BACKEND_MEMORY_PREFIX)) == 0) {
		key = make_file_name((char*)in_key);
		snprintf(initpath, BUFSIZE, "%s%s", MEMORY_INIT, key);
		if(key) free(key);
		_vconf_remove_file(initpath);
	}

	return ret;
}

static int _vconf_backup(char *in_key, const char* backup_dir)
{
	char path[BUFSIZE] = {0,};
	int ret = 0;
	char szCmd[BUFSIZE] = {0,};
	char *key = NULL;
	char *smack_access_label  = NULL;
	char destfile[BUFSIZE] = {0,};

	retvm_if(in_key == NULL, VCONF_ERROR, "[vconf] Invalid argument: key is null");
	retvm_if(backup_dir == NULL, VCONF_ERROR, "[vconf] Invalid argument: directory is null");

	if (getuid() != 0) {
		fprintf(stderr, "[vconf] Only root user can use backup command\n");
		return -1;
	}

	retvm_if(access(backup_dir, F_OK) == -1,
		VCONF_ERROR, "[vconf] Error : backup directory(%s) is not exist", backup_dir);

	ret = smack_getlabel(backup_dir, &smack_access_label, SMACK_LABEL_ACCESS);
	if(ret != 0) {
		fprintf(stderr, "[vconf] Err(%d) in getting smack label of backup directory\n", ret);
		return -1;
	}
	if((smack_access_label == NULL) || (strncmp(smack_access_label,"_",1) != 0)) {
		fprintf(stderr, "[vconf] smack label of backup directory is not permitted\n");
		if(smack_access_label) free(smack_access_label);
		return -1;
	}
	free(smack_access_label);

	ret = make_file_path(in_key, path);
	retvm_if(ret != VCONF_OK, VCONF_ERROR, "[vconf] Invalid argument: key(%s) is not valid", in_key);

	retvm_if(access(path, F_OK) == -1, VCONF_ERROR, "[vconf] Error : key(%s) is not exist", in_key);

	key = make_file_name(in_key);
	snprintf(destfile, BUFSIZE, "%s/%s", backup_dir, key);
	if(key) free(key);

	/* Check directory & create it */
	if(_check_parent_dir(destfile)) {
		fprintf(stderr, "[vconf] fail to mkdir (%s,%s) \n", backup_dir, in_key);
		return -1;
	}

	/* cp */
	snprintf(szCmd, BUFSIZE, "/bin/cp %s %s -af", path, destfile);
	if (__system(szCmd)) {
		fprintf(stderr, "[vconf]vconf backup cmd error() key=%s / dir=%s\n", in_key, backup_dir);
		return -1;
	}

	/* set smack label (--preserve=all is not working at mic) */
	ret = smack_getlabel(path, &smack_access_label, SMACK_LABEL_ACCESS);
	if(ret != 0) {
		fprintf(stderr, "[vconf] Err(%d) in getting smack label of backup key\n", ret);
		return -1;
	}

	if(smack_access_label) {
		if (smack_setlabel(destfile, smack_access_label, SMACK_LABEL_ACCESS) < 0) {
			if (errno != EOPNOTSUPP) {
				fprintf(stderr, "[vconf][smack] smack set label (%s) failed (%s - %d)\n", smack_access_label, destfile, errno);
				return -1;
			}
		}
		free(smack_access_label);
	}

	return 0;
}

static int _vconf_restore(char *in_key, const char* restore_dir)
{
	char path[BUFSIZE] = {0,};
	int ret = 0;
	char szCmd[BUFSIZE] = {0,};
	char *key = NULL;
	char *smack_access_label  = NULL;
	char source[BUFSIZE] = {0,};
	char dest[BUFSIZE] = {0,};
	int memory_key = 0;
	int func_ret = 0;

	retvm_if(in_key == NULL, VCONF_ERROR, "[vconf] Invalid argument: key is null");
	retvm_if(restore_dir == NULL, VCONF_ERROR, "[vconf] Invalid argument: directory is null");

	if (getuid() != 0) {
		fprintf(stderr, "[vconf] Only root user can use backup command\n");
		return -1;
	}

	retvm_if(access(restore_dir, F_OK) == -1, VCONF_ERROR, "[vconf] Error : restore directory(%s) is not exist", restore_dir);

	ret = smack_getlabel(restore_dir, &smack_access_label, SMACK_LABEL_ACCESS);
	if(ret != 0) {
		fprintf(stderr, "[vconf] Err(%d) in getting smack label of backup directory\n", ret);
		return -1;
	}
	if((smack_access_label == NULL) || (strncmp(smack_access_label,"_",1) != 0)) {
		fprintf(stderr, "[vconf] smack label of backup directory is not permitted\n");
		if(smack_access_label) free(smack_access_label);
		return -1;
	}
	free(smack_access_label);

	if(strncmp(in_key, VCONF_RESTORE_ALL, strlen(VCONF_RESTORE_ALL))== 0) {
		snprintf(source, BUFSIZE, "%s/%s", restore_dir, BACKEND_DB_PREFIX);
		if(access(source, F_OK) == 0) {
			snprintf(szCmd, BUFSIZE, "/bin/cp %s* %s%s -r --preserve=all", source, DB_PREFIX, BACKEND_DB_PREFIX);
			if (__system(szCmd)) {
				fprintf(stderr, "[vconf]vconf restore db key error() dir=%s\n", restore_dir);
				return -1;
			}
		}
		memset(source, 0x00, BUFSIZE);
		memset(szCmd, 0x00, BUFSIZE);
		snprintf(source, BUFSIZE, "%s/%s", restore_dir, BACKEND_FILE_PREFIX);
		if(access(source, F_OK) == 0) {
			snprintf(szCmd, BUFSIZE, "/bin/cp %s* %s%s -r --preserve=all", source, FILE_PREFIX, BACKEND_FILE_PREFIX);
			if (__system(szCmd)) {
				fprintf(stderr, "[vconf]vconf restore db key error() dir=%s\n", restore_dir);
				return -1;
			}
		}
		memset(source, 0x00, BUFSIZE);
		memset(szCmd, 0x00, BUFSIZE);
		snprintf(source, BUFSIZE, "%s/%s", restore_dir, BACKEND_MEMORY_PREFIX);
		if(access(source, F_OK) == 0) {
			snprintf(szCmd, BUFSIZE, "/bin/cp %s* %s%s -r --preserve=all", source, MEMORY_INIT, BACKEND_MEMORY_PREFIX);
			if (__system(szCmd)) {
				fprintf(stderr, "[vconf]vconf restore db key error() dir=%s\n", restore_dir);
				return -1;
			}
			memset(szCmd, 0x00, BUFSIZE);
			snprintf(szCmd, BUFSIZE, "/bin/cp %s* %s%s -r --preserve=all", source, MEMORY_PREFIX, BACKEND_MEMORY_PREFIX);
			if (__system(szCmd)) {
				fprintf(stderr, "[vconf]vconf restore db key error() dir=%s\n", restore_dir);
				return -1;
			}
		}
	} else {
		ret = make_file_path(in_key, path);
		retvm_if(ret != VCONF_OK, VCONF_ERROR, "[vconf] Invalid argument: key(%s) is not valid", in_key);
		retvm_if(access(path, F_OK) == -1, VCONF_ERROR, "[vconf] Error : key(%s) is not exist", in_key);

		key = make_file_name(in_key);
		snprintf(source, BUFSIZE, "%s/%s", restore_dir, key);

		if(access(source, F_OK) == 0) {

			if (strncmp(in_key, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1) == 0) {
				snprintf(dest, BUFSIZE, "%s/%s", DB_PREFIX, key);
			} else if (strncmp(in_key, BACKEND_FILE_PREFIX, sizeof(BACKEND_FILE_PREFIX) - 1) == 0) {
				snprintf(dest, BUFSIZE, "%s/%s", FILE_PREFIX, key);
			} else if (strncmp(in_key, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1) == 0) {
				snprintf(dest, BUFSIZE, "%s/%s", MEMORY_INIT, key);
				memory_key = 1;
			}

			/* Check directory & create it */
			if(_check_parent_dir(dest)) {
				fprintf(stderr, "[vconf] fail to mkdir (%s,%s) \n", dest, in_key);
				func_ret = -1;
				goto out_func;
			}

			memset(szCmd,0x00,BUFSIZE);
			snprintf(szCmd, BUFSIZE, "/bin/cp %s %s --preserve=all", source, dest);
			if (__system(szCmd)) {
				fprintf(stderr, "[vconf]vconf restore key error() dir=%s\n", restore_dir);
				func_ret = -1;
				goto out_func;
			}

			if(memory_key) {
				memset(szCmd,0x00,BUFSIZE);
				snprintf(szCmd, BUFSIZE, "/bin/cp %s %s%s --preserve=all", source, MEMORY_PREFIX, BACKEND_MEMORY_PREFIX);
				if (__system(szCmd)) {
					fprintf(stderr, "[vconf]vconf restore key error() dir=%s\n", restore_dir);
					func_ret = -1;
					goto out_func;
				}
			}
		}

out_func :
		if(key) free(key);
	}

	return func_ret;
}

int main(int argc, char **argv)
{
	int set_type;
	char szFilePath[BUFSIZE] = { 0, };
	char *psz_key = NULL;

	GError *error = NULL;
	GOptionContext *context = NULL;

#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 36)
	g_type_init();
#endif
	context = g_option_context_new("- vconf library tool");

	if (context == NULL)
	{
		g_print("g_option_context_new has returned null\n");
		exit(1);
	}

	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_help_enabled(context, FALSE);
	g_option_context_set_ignore_unknown_options(context, TRUE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		g_option_context_free(context);
		exit(1);
	}

	if (argc < 2) {
		print_help(argv[0]);
		g_option_context_free(context);
		return 1;
	}

	if (!strncmp(argv[1], "set", 3)) {
		set_type = check_type();
		if (argc < 4 || !set_type) {
			print_help(argv[0]);
			g_option_context_free(context);
			return 1;
		}

		if (smack_label) {
			if (getuid() != 0) {
				fprintf(stderr,
					"[vconf] Only root user can use '-s' option\n");
				g_option_context_free(context);
				return -1;
			}
		}

		psz_key = argv[2];

		if (make_file_path(psz_key, szFilePath)) {
			fprintf(stderr, "[vconf] Bad prefix\n");
			g_option_context_free(context);
			return -1;
		}

		if (check_file_path_mode(psz_key, szFilePath)) {
			g_option_context_free(context);
			return -1;
		}

		switch (set_type) {
			case VCONFTOOL_TYPE_STRING:
				vconf_set_str(psz_key, argv[3]);
				break;
			case VCONFTOOL_TYPE_INT:
				vconf_set_int(psz_key, atoi(argv[3]));
				break;
			case VCONFTOOL_TYPE_DOUBLE:
				vconf_set_dbl(psz_key, atof(argv[3]));
				break;
			case VCONFTOOL_TYPE_BOOL:
				vconf_set_bool(psz_key, !!atoi(argv[3]));
				break;
			default:
				fprintf(stderr, "[vconf] set type never reach");
				g_option_context_free(context);
				exit(1);
		}

		/* Install memory backend key into flash space *******/
		if (is_initialization) {
			if (0 == strncmp(psz_key, BACKEND_MEMORY_PREFIX, sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
				_vconf_backup(psz_key, MEMORY_INIT);
			}
		}
		/* End memory backend key into flash space ***********/
	} else if (!strncmp(argv[1], "get", 3)) {
		if (argv[2])
			get_operation(argv[2]);
		else
			print_help(argv[0]);
	} else if (!strncmp(argv[1], "unset", 5)) {
		if (argv[2]) {
			_vconf_unset(argv[2]);
		}
		else
			print_help(argv[0]);
	} else if (!strncmp(argv[1], "backup", 6)) {
		if ((argv[2]) && (argv[3])) {
			_vconf_backup(argv[2],argv[3]);
		}
		else
			print_help(argv[0]);
	} else if (!strncmp(argv[1], "restore", 7)) {
		if ((argv[2]) && (argv[3])) {
			_vconf_restore(argv[2],argv[3]);
		}
		else
			print_help(argv[0]);
	} else
		fprintf(stderr, "[vconf] %s is a invalid command\n", argv[1]);
	g_option_context_free(context);
	return 0;
}

static void get_operation(char *input)
{
	keylist_t *get_keylist;
	keynode_t *temp_node;
	char *test;

	get_keylist = vconf_keylist_new();
	/* ParentDIR parameter of gconf_client_all_entries
	can not include the last slash. */
	if ('/' == input[strlen(input) - 1] && strlen(input) > 8)
		input[strlen(input) - 1] = '\0';

	vconf_get(get_keylist, input, VCONF_GET_KEY);
	if (!(temp_node = vconf_keylist_nextnode(get_keylist))) {
		test = strrchr(input, '/');
		if (NULL != test) {
			vconf_keylist_add_null(get_keylist, input);
			if (test - input < 7)
				*(test + 1) = '\0';
			else
				*test = '\0';
			if(vconf_get(get_keylist, input, VCONF_GET_KEY)!=VCONF_OK) {
				fprintf(stderr, "[vconf] error in get vconf(%s) value\n", input);
			}
			temp_node = vconf_keylist_nextnode(get_keylist);
		} else {
			fprintf(stderr, "[vconf] Include at least one slash\"/\"\n");
			vconf_keylist_free(get_keylist);
			return;
		}
	}
	get_num = 0;
	print_keylist(get_keylist, temp_node, 0);

	if (!get_num)
		fprintf(stderr, "[vconf] No data\n");
	vconf_keylist_free(get_keylist);
}

static void recursive_get(char *subDIR, int level)
{
	printf("%s", subDIR);

	keylist_t *get_keylist;
	keynode_t *first_node;

	get_keylist = vconf_keylist_new();
	vconf_get(get_keylist, subDIR, VCONF_GET_ALL);

	if ((first_node = vconf_keylist_nextnode(get_keylist))) {
		print_keylist(get_keylist, first_node, level);
	}
	vconf_keylist_free(get_keylist);
}

static void print_keylist(keylist_t *keylist, keynode_t *temp_node, int level)
{
	do {
		switch (vconf_keynode_get_type(temp_node))
		{
			case VCONF_TYPE_INT:
				printf("%s, value = %d (int)\n",
				       vconf_keynode_get_name(temp_node),
				       vconf_keynode_get_int(temp_node));
				get_num++;
				break;
			case VCONF_TYPE_BOOL:
				printf("%s, value = %d (bool)\n",
				       vconf_keynode_get_name(temp_node),
				       vconf_keynode_get_bool(temp_node));
				get_num++;
				break;
			case VCONF_TYPE_DOUBLE:
				printf("%s, value = %.16lf (double)\n",
				       vconf_keynode_get_name(temp_node),
				       vconf_keynode_get_dbl(temp_node));
				get_num++;
				break;
			case VCONF_TYPE_STRING:
				printf("%s, value = %s (string)\n",
				       vconf_keynode_get_name(temp_node),
				       vconf_keynode_get_str(temp_node));
				get_num++;
				break;
			case VCONF_TYPE_DIR:
				printf("%s(Directory)\n",
					vconf_keynode_get_name(temp_node));
				if (is_recursive)
					recursive_get(vconf_keynode_get_name(temp_node),
						      level + 1);
				break;
			default:
				/* fprintf(stderr, "Unknown Type(%d)\n", vconf_keynode_get_type(temp_node)); */
				break;
		}
	} while ((temp_node = vconf_keylist_nextnode(keylist)));
}
