#ifndef _CUST_CHARGING_H_
#define _CUST_CHARGING_H_

#ifdef MTK_BQ24196_SUPPORT
#define BQ24196_BUSNUM 3
#endif

#ifdef MTK_BQ24158_SUPPORT
#define BQ24158_BUSNUM 3
#endif

#ifdef MTK_BQ24296_SUPPORT
#define BQ24296_BUSNUM 3
#endif

#ifdef MTK_FAN5405_SUPPORT
#define FAN5405_BUSNUM 3
#endif

#ifdef MTK_NCP1854_SUPPORT
#define NCP1854_BUSNUM 3
#endif

//#define NCP1854_PWR_PATH 1  //disable power path because HQA fail

#endif //_CUST_CHARGING_H_
