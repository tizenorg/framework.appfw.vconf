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
#include "vconf-log.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/smack.h>
#include <fcntl.h>
#include <errno.h>

#include <wordexp.h>

enum {
	VCONFTOOL_TYPE_NO = 0x00,
	VCONFTOOL_TYPE_STRING,
	VCONFTOOL_TYPE_INT,
	VCONFTOOL_TYPE_DOUBLE,
	VCONFTOOL_TYPE_BOOL
};

#define BUFSIZE		1024

const char *BACKEND_DB_PREFIX = "db/";
const char *BACKEND_FILE_PREFIX = "file/";
const char *BACKEND_MEMORY_PREFIX = "memory/";

const char *DB_PREFIX = "/opt/var/kdb";
const char *FILE_PREFIX = "/opt/var/kdb";
const char *MEMORY_PREFIX = "/var/run";
const char *MEMORY_INIT = "/opt/var/kdb/memory_init";

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
	fprintf(stderr,
		"           Ex) %s set -t string db/testapp/key1 \"This is test\" -s system::vconf_setting\n",
		cmd);
	fprintf(stderr,
		"           NOTE: If -s option is not used, the default smack access label system::vconf will be set.\n");
	fprintf(stderr,
		"           NOTE: Maximum label size is 255 byte and '/', '~', ' ' is not permitted.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "[Get vconf value]\n");
	fprintf(stderr, "       %s get <OPTIONS> <KEY NAME>\n", cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "       <OPTIONS>\n");
	fprintf(stderr,
		"          -r : retrieve all keys included in sub-directorys \n");
	fprintf(stderr, "       Ex) %s get db/testapp/key1\n", cmd);
	fprintf(stderr, "           %s get db/testapp/\n", cmd);
	fprintf(stderr, "\n");
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
			perror("waitpid failed");
			return -1;
		}
		if (WIFSIGNALED(status)) {
			printf("signal(%d)\n", WTERMSIG(status));
			perror("exit by signal");
			return -1;
		}
		if (!WIFEXITED(status)) {
			perror("exit abnormally");
			return -1;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status)) {
			perror("child return error");
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
			fprintf(stderr,"invalid char is found\n");
			*tmp = '_';
		}
	}
}

static int check_file_path_mode(char* file_path)
{
	char szCmd[BUFSIZE] = {0,};
	int create_file = 0;
	int set_id = 0;
	int fd;

	if (guid || uid) {
		if (getuid() != 0) {
			fprintf(stderr,
				"Error!\t Only root user can use '-g or -u' option\n");
			return -1;
		}

		set_id = 1;
	}

	/* Check file path */
	if (access(file_path, F_OK) != 0) {
		/* fprintf(stderr,"key does not exist\n"); */

		char szPath[BUFSIZE] = { 0, };
		char *pCh = strrchr(file_path, '/');
		strncat(szPath, file_path, pCh - file_path);
		/* fprintf(stderr, "szPath : %s\n", szPath); */

		/* Check directory & create it */
		if (access(szPath, F_OK) != 0) {
			/* fprintf(stderr,"parent dir does not exist\n"); */

			snprintf(szCmd, BUFSIZE, "/bin/mkdir %s -p --mode=755", szPath);
			disable_invalid_char(szCmd);
			if (__system(szCmd)) {
				fprintf(stderr,"[%s:%d]Fail mkdir() szCmd=%s\n", __FILE__, __LINE__, szCmd);
				return -1;
			}

		}

		create_file = 1;
	} else if (!is_forced) {
		fprintf(stderr, "Key already exist. Use -f option to force update\n");
		return -1;
	}

	if(create_file) {
		/* Create file */
		mode_t temp;
		temp = umask(0000);
		fd = open(file_path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
		umask(temp);
		if(fd == -1) {
			fprintf(stderr, "open(rdwr,create) error\n");
			return -1;
		}
		close(fd);
	}

	if(set_id) {
		if((fd = open(file_path, O_RDONLY)) == -1) {
			fprintf(stderr, "open(rdonly) error\n");
			return -1;
		}

		if(uid) {
			if (fchown(fd, atoi(uid), atoi(uid)) == -1) {
				fprintf(stderr, "Error!\t Fail to fchown(%d)\n", errno);
				close(fd);
				return -1;
			}
		} else if (guid) {
			if (-1 == fchown(fd, 0, atoi(guid))) {
				fprintf(stderr, "Error!\t Fail to fchown()\n");
				close(fd);
				return -1;
			}
		}

		close(fd);
	}

	return 0;
}

/*
 * there are three different types of vconf key back-end
 * 1. filesytem
 * 2. sqlitefs based on sqlite3
 * 3. tmpfs based on memory
 * considering to that tmpfs is volatile so there should be a way
 * to initialize key into permanent storage. That is the below function
 * which copy memory back-end key to filesystem back-end, copy other back-end
 * key to filesystem back-end just makes no sense.
 */
static int copy_memory_key(char *pszKey, char *pszOrigin)
{
	char szCmd[BUFSIZE] = { 0, };
	char szPath[BUFSIZE] = { 0, };
	char szFileName[BUFSIZE] = { 0, };
	char *pCh = strrchr(pszKey, '/');
	int nLen = strlen(pszKey);

	/* only copy memory/ prefix key */
	if (strncmp("memory/", pszKey, 7))
		return 0;

	/* Get directory path and file name */
	snprintf(szPath, BUFSIZE, "%s/", MEMORY_INIT);
	strncat(szPath, pszKey, pCh - pszKey);
	strncpy(szFileName, pszKey + (pCh - pszKey) + 1, nLen - (pCh - pszKey));

	/* Check directory & create it */
	if (0 != access(szPath, F_OK)) {
		snprintf(szCmd, BUFSIZE, "mkdir %s -p --mode=755", szPath);
		if (system(szCmd)) {
			printf("[%s:%d]Fail mkdir() szCmd=%s\n", __FILE__,
			       __LINE__, szCmd);
			return -1;
		}
	}
	/* copy */
	strncat(szPath, "/", 1);
	strncat(szPath, szFileName, strlen(szFileName));
	memset(szCmd, 0x00, BUFSIZE);
	snprintf(szCmd, BUFSIZE, "cp %s %s -r -p", pszOrigin, szPath);
	if (system(szCmd)) {
		printf("[%s:%d]Fail copy\n", __FILE__, __LINE__);
		return -1;
	}

	if (smack_label) {
		if (smack_setlabel(szPath, smack_label, SMACK_LABEL_ACCESS) < 0) {
			if (errno != EOPNOTSUPP) {
				fprintf(stderr, "smack set label (%s) failed (%s)\n", smack_label, szPath);
				return -1;
			}
		}
	}

	return 0;
}

static int make_file_path(char *pszKey, char *pszBuf)
{
	if (0 ==
	    strncmp(pszKey, BACKEND_DB_PREFIX, sizeof(BACKEND_DB_PREFIX) - 1)) {
		snprintf(pszBuf, BUFSIZE, "%s/%s", DB_PREFIX, pszKey);
		return 0;
	} else if (0 ==
		   strncmp(pszKey, BACKEND_FILE_PREFIX,
			   sizeof(BACKEND_FILE_PREFIX) - 1)) {
		snprintf(pszBuf, BUFSIZE, "%s/%s", FILE_PREFIX, pszKey);
		return 0;
	} else if (0 ==
		   strncmp(pszKey, BACKEND_MEMORY_PREFIX,
			   sizeof(BACKEND_MEMORY_PREFIX) - 1)) {
		snprintf(pszBuf, BUFSIZE, "%s/%s", MEMORY_PREFIX, pszKey);
		return 0;
	}
	return -1;
}

int main(int argc, char **argv)
{
	int set_type;
	char szFilePath[BUFSIZE] = { 0, };
	char *psz_key = NULL;

	GError *error = NULL;
	GOptionContext *context;

	g_type_init();
	context = g_option_context_new("- vconf library tool");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_help_enabled(context, FALSE);
	g_option_context_set_ignore_unknown_options(context, TRUE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		exit(1);
	}

	if (argc < 2) {
		print_help(argv[0]);
		return 1;
	}

	if (!strncmp(argv[1], "set", 3)) {
		set_type = check_type();
		if (argc < 4 || !set_type) {
			print_help(argv[0]);
			return 1;
		}

		if (make_file_path(argv[2], szFilePath)) {
			fprintf(stderr, "Error!\t Bad prefix\n");
			return -1;
		}

		if (check_file_path_mode(szFilePath)) {
			fprintf(stderr, "Error!\t create key %s\n", argv[2]);
			return -1;
		}

		if (smack_label) {
			if (getuid() != 0) {
				fprintf(stderr,
					"Error!\t Only root user can use '-s' option\n");
				return -1;
			}
			if (smack_setlabel(szFilePath, smack_label, SMACK_LABEL_ACCESS) < 0) {
				if (errno != EOPNOTSUPP) {
					fprintf(stderr, "smack set label (%s) failed (%s)\n", smack_label, szFilePath);
					return -1;
				}
			}
		}

		switch (set_type) {
			case VCONFTOOL_TYPE_STRING:
				vconf_set_str(argv[2], argv[3]);
				break;
			case VCONFTOOL_TYPE_INT:
				vconf_set_int(argv[2], atoi(argv[3]));
				break;
			case VCONFTOOL_TYPE_DOUBLE:
				vconf_set_dbl(argv[2], atof(argv[3]));
				break;
			case VCONFTOOL_TYPE_BOOL:
				vconf_set_bool(argv[2], !!atoi(argv[3]));
				break;
			default:
				fprintf(stderr, "never reach");
				exit(1);
		}

		psz_key = argv[2];
		/* Install memory backend key into flash space *******/
		if (is_initialization) {
			copy_memory_key(psz_key, szFilePath);
		}
		/* End memory backend key into flash space ***********/

	} else if (!strncmp(argv[1], "get", 3)) {
		if (argv[2])
			get_operation(argv[2]);
		else
			print_help(argv[0]);
	} else if (!strncmp(argv[1], "unset", 5)) {
		if (argv[2])
			vconf_unset(argv[2]);
		else
			print_help(argv[0]);
	} else
		fprintf(stderr, "%s is a invalid command\n", argv[1]);
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

	vconf_get(get_keylist, input, VCONF_GET_ALL);
	if (!(temp_node = vconf_keylist_nextnode(get_keylist))) {
		test = strrchr(input, '/');
		if (NULL != test) {
			vconf_keylist_add_null(get_keylist, input);
			if (test - input < 7)
				*(test + 1) = '\0';
			else
				*test = '\0';
			vconf_get(get_keylist, input, VCONF_GET_KEY);
			temp_node = vconf_keylist_nextnode(get_keylist);
		} else {
			fprintf(stderr, "Include at least one slash\"/\"\n");
			vconf_keylist_free(get_keylist);
			return;
		}
	}
	get_num = 0;
	print_keylist(get_keylist, temp_node, 0);

	if (!get_num)
		printf("No data\n");
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
				printf("%s, value = %f (double)\n",
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
