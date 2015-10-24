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

#ifndef __VCONF_LOG_H__
#define __VCONF_LOG_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define VCONF_DLOG 1
#define VCONF_SYSLOG 2
#define VCONF_PRINTLOG 3

#ifndef VCONF_LOG
#define VCONF_LOG VCONF_DLOG
#endif

/************** Error ***************/
#if VCONF_LOG == VCONF_SYSLOG
	#include <syslog.h>

	#define INFO(fmt, arg...) \
		do { \
			syslog(LOG_INFO, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
	#define DBG(fmt, arg...) \
		do { \
			syslog(LOG_INFO, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
	#define ERR(fmt, arg...) \
		do { \
			syslog(LOG_ERR, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
	#define FATAL(fmt, arg...) \
		do { \
			syslog(LOG_ERR, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
	#define WARN(fmt, arg...) \
		do { \
			syslog(LOG_ERR, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)

#elif VCONF_LOG == VCONF_DLOG
	#define LOG_TAG	"VCONF"
	#include <dlog.h>

	#define INFO(fmt, arg...)
	#define DBG(fmt, arg...) SECURE_SLOGI(fmt, ##arg)
	#define ERR(fmt, arg...) LOGE(fmt, ##arg)
	#define SECURE_ERR(fmt, arg...) SECURE_SLOGE(fmt, ##arg)
	#define FATAL(fmt, arg...) SECURE_SLOGF(fmt, ##arg)
	#define WARN(fmt, arg...) SECURE_SLOGW(fmt, ##arg)

#elif VCONF_LOG == VCONF_PRINTLOG
	#include <stdlib.h>

	#define INFO(fmt, arg...)
	#define DBG(fmt, arg...) \
		do { \
			fprintf(stdout,fmt"\n", ##arg); \
		}while(0)
	#define WARN(fmt, arg...) \
		do { \
			fprintf(stderr,fmt"\n", ##arg); \
		}while(0)
	#define ERR(fmt, arg...) \
		do { \
			fprintf(stderr,fmt"\n", ##arg); \
		}while(0)
	#define FATAL(fmt, arg...) \
		do { \
			fprintf(stderr,fmt"\n", ##arg); \
		}while(0)
#endif


/************** Return ***************/
#define ret_if(expr) \
	do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#define retv_if(expr, val) \
	do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)
#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0)
#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0)
#define retex_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			goto CATCH; \
		} \
	} while (0)


/************** TimeCheck ***************/
#ifdef VCONF_TIMECHECK
#define START_TIME_CHECK \
	init_time();\
	startT = set_start_time();
#define END_TIME_CHECK \
	VCONF_DEBUG("time = %f ms\n", exec_time(startT));
#else
#define START_TIME_CHECK
#define END_TIME_CHECK
#endif


#endif				/* __VCONF_LOG_H__ */
