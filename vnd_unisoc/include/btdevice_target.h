
/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
 *
 ******************************************************************************/

#ifndef BTDEVICE_TARGET_H
#define BTDEVICE_TARGET_H

#include "bt_types.h" /* This must be defined AFTER buildcfg.h */

#define HOST_DEVICE_COEXISTENCE

#undef BTA_AV_SINK_INCLUDED
#ifndef BTA_AV_SINK_INCLUDED
#define BTA_AV_SINK_INCLUDED TRUE
#endif

#ifndef BTIF_HF_CLIENT_FEATURES
#define BTIF_HF_CLIENT_FEATURES                                                \
  (BTA_HF_CLIENT_FEAT_ECNR | BTA_HF_CLIENT_FEAT_3WAY |                         \
   BTA_HF_CLIENT_FEAT_CLI | BTA_HF_CLIENT_FEAT_VREC | BTA_HF_CLIENT_FEAT_VOL | \
   BTA_HF_CLIENT_FEAT_ECS | BTA_HF_CLIENT_FEAT_ECC )
#endif

//#ifndef DISABLE_WBS
//#define DISABLE_WBS FALSE
//#endif

/* The default scan mode */
//#ifndef BTM_DEFAULT_SCAN_TYPE
//#define BTM_DEFAULT_SCAN_TYPE BTM_SCAN_TYPE_INTERLACED
//#endif


/* Default class of device
* {SERVICE_CLASS, MAJOR_CLASS, MINOR_CLASS}
*
* SERVICE_CLASS:0x5A (Bit17 -Networking,Bit19 - Capturing,Bit20 -Object
* Transfer,Bit22 -Telephony)
* MAJOR_CLASS:0x02 - PHONE
* MINOR_CLASS:0x0C - SMART_PHONE
*
*/
#undef BTA_DM_COD
#ifndef BTA_DM_COD
#define BTA_DM_COD \
  { 0x5A, 0x04, 0x08 }
#endif

/* Fixed Default String. When this is defined as null string, the device's
 * product model name is used as the default local name.
 */
 #undef BTM_DEF_LOCAL_NAME
#ifndef BTM_DEF_LOCAL_NAME
#define BTM_DEF_LOCAL_NAME "carkit_bt"
#endif

/* The maximum length, in bytes, of an attribute. */
 #undef SDP_MAX_ATTR_LEN
#ifndef SDP_MAX_ATTR_LEN
#ifdef BTA_AV_SINK_INCLUDED
#define SDP_MAX_ATTR_LEN 2000
#else
#define SDP_MAX_ATTR_LEN 400
#endif
#endif


#endif /* BTDEVICE_TARGET_H */
