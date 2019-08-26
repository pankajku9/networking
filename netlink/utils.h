/*
 * utils.h
 *
 *  Created on: Jul 27, 2019
 *      Author: pankajku
 */

#ifndef NETLINK_EX1_UTILS_H_
#define NETLINK_EX1_UTILS_H_

/* DIACTOMY OF C: 0 if success, NULL is failure */
#define TRACE_DIS    1

#if(TRACE_DIS)
#define ret_on_err(errno, msg) do { if (errno < 0) {  \
			printf("%s %d : %s failed \n", __FUNCTION__, __LINE__, msg); \
			return errno; } }  while(0)
#else
#define ret_on_err(errno, msg) do { if (errno < 0) { \
			printf("%s %d : %s\n", __FUNCTION__, __LINE__, msg); \
			return errno; } else { \
			printf("FUNC:%s LINE:%d %s passed \n", __FUNCTION__, __LINE__, msg); } \
			}  while(0)
#endif


#if(TRACE_DIS)
#define goto_on_err(label, errno, msg) do { if (errno < 0) {  \
			printf("%s %d : %s failed\n", __FUNCTION__, __LINE__, msg); \
			goto label; } }  while(0)
#else
#define goto_on_err(label, errno, msg) do { if (errno < 0) { \
			printf("%s %d : %s\n", __FUNCTION__, __LINE__, msg); \
			goto label; } else { \
			printf("FUNC:%s LINE:%d %s passed \n", __FUNCTION__, __LINE__, msg); } \
			}  while(0)
#endif


#if(TRACE_DIS)
#define ret_on_null(ptr, msg) do { if (!ptr) {  \
			printf("%s %d : %s\n", __FUNCTION__, __LINE__, msg); \
			return -1; } }  while(0)
#else
#define ret_on_null(ptr, msg) do { if (!ptr) { \
			printf("%s %d : %s failed\n", __FUNCTION__, __LINE__, msg); \
			return ptr; } else { \
			printf("FUNC:%s LINE:%d %s passed \n", __FUNCTION__, __LINE__, msg); } \
			}  while(0)
#endif


#if(TRACE_DIS)
#define goto_on_null(label, ptr, msg) do { if (!ptr)) {  \
			printf("%s %d : %s failed \n", __FUNCTION__, __LINE__, msg); \
			goto label; } }  while(0)
#else
#define goto_on_null(label, ptr, msg) do { if (!ptr) { \
			printf("%s %d : %s\n", __FUNCTION__, __LINE__, msg); \
			goto label; } else { \
			printf("FUNC:%s LINE:%d %s passed \n", __FUNCTION__, __LINE__, msg); } \
			}  while(0)
#endif


#endif


