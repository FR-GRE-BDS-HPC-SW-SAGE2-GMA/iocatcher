/* -*- C -*- */
/*
 * Copyright (c) 2017-2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 *
 * Original author:  Ganesan Umanesan <ganesan.umanesan@seagate.com>
 * Original creation date: 26-May-2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdarg.h>

#define PBUFSZ 1024

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
extern int perf; 	/* performance 		*/

/*
 ******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 ******************************************************************************
 */

static struct  timeval wclk_t = {0, 0};
static clock_t cput_t = 0;
static char pbuf[PBUFSZ] = {0};

/*
 ******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************
 */

/*
 * c0appz_dump_perf()
 * display/store performance results
 */
int c0appz_dump_perf(void)
{
	printf("%s",pbuf);
	return 0;
}

/*
 * ppf()
 * Append formatted string to
 * the performance buffer
 */
int ppf(const char *fmt, ...)
{
	int n=0;
	int s=0;
	char buffer[128];
	if(!perf) return 0;
	va_list args;
	va_start(args, fmt);
	s = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	n = strlen(pbuf);
	snprintf(pbuf+n,PBUFSZ-n,buffer);
	return s;
}

/*
 * c0appz_timeout()
 * time out execution.
 */
int c0appz_timeout(uint64_t sz)
{
	double ct;        /* cpu time in seconds  */
	double wt;        /* wall time in seconds */
	double bw_ctime;  /* bandwidth in MBs     */
	double bw_wtime;  /* bandwidth in MBs     */
	struct timeval tv;

	if(!perf) return 0;

	/* cpu time */
	ct = (double)(clock() - cput_t) / CLOCKS_PER_SEC;
	bw_ctime = (double)(sz) / 1000000.0 / ct;

	/* wall time */
	gettimeofday(&tv, 0);
	wt  = (double)(tv.tv_sec - wclk_t.tv_sec);
	wt += (double)(tv.tv_usec - wclk_t.tv_usec)/1000000;
	bw_wtime  = (double)(sz) / 1000000.0 / wt;

/*
	fprintf(stderr,"[ cput: %10.4lf s %10.4lf MB/s ]", ct, bw_ctime);
	fprintf(stderr,"[ wclk: %10.4lf s %10.4lf MB/s ]", wt, bw_wtime);
	fprintf(stderr,"\n");
*/
	ppf("[ cput: %10.4lf s %10.4lf MB/s ]", ct, bw_ctime);
	ppf("[ wclk: %10.4lf s \033[0;31m%10.4lf MB/s\033[0m ]", wt, bw_wtime);
	ppf("\n");
	return 0;
}

/*
 * c0appz_timein()
 * time in execution.
 */
int c0appz_timein()
{
	cput_t = clock();
	gettimeofday(&wclk_t, 0);
	return 0;
}

/*
 ******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************
 */


/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
