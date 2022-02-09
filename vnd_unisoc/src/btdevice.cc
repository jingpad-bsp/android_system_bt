/******************************************************************************
 *
 *   Copyright (C) 2019 Spreadtrum Communication Inc.
 *
 ******************************************************************************/

#define LOG_TAG "btdevice"

#include <base/logging.h>
#include <string.h>

#include <hardware/bluetooth.h>
#include <hardware/bt_av.h>
#include <hardware/bt_rc.h>
#include "sdpint.h"
#include "bt_common.h"
#include "btdevice.h"
#include "avdt_api.h"
#include "stack/include/avrc_defs.h"
#include "stack/include/avrc_api.h"
#include "btif_rc.h"
#include "btif_av.h"
#include "btif_storage.h"
#include <bluetooth/uuid.h>
/*****************************************************************************
 *  Constants & Macros
 *****************************************************************************/

/*****************************************************************************
 *  Local type definitions
 *****************************************************************************/
using bluetooth::Uuid;

/*******************************************************************************
 *  Static variables
 ******************************************************************************/
static const uint8_t sdp_base_uuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x10, 0x00, 0x80, 0x00, 0x00, 0x80,
                                        0x5F, 0x9B, 0x34, 0xFB};
static volatile uint8_t btif_local_tsep = AVDT_TSEP_INVALID;
uint8_t src_uuid[2] = {0X11, 0X0A};//UUID_SERVCLASS_AUDIO_SOURCE;
uint8_t sink_uuid[2] = {0X11, 0X0B}; //= UUID_SERVCLASS_AUDIO_SINK;

#define BTDEVICE_STORAGE_GET_REMOTE_PROP(b, t, v, l, p)     \
  do {                                                  \
    (p).type = (t);                                     \
    (p).val = (v);                                      \
    (p).len = (l);                                      \
    btif_storage_get_remote_device_property((b), &(p)); \
  } while (0)

/******************************************************************************
 *  Functions
 *****************************************************************************/
 /*******************************************************************************
 *
 * Function         compare_uuid_arrays
 *
 * Description      This function compares 2 BE UUIDs. If needed, they are
 *                  expanded to 128-bit UUIDs, then compared.
 *
 * NOTE             it is assumed that the arrays are in Big Endian format
 *
 * Returns          true if matched, else false
 *
 ******************************************************************************/
static bool compare_uuid_arrays(uint8_t* p_uuid1, uint32_t len1, uint8_t* p_uuid2,
                              uint16_t len2) {
  uint8_t nu1[Uuid::kNumBytes128];
  uint8_t nu2[Uuid::kNumBytes128];

  if (((len1 != 2) && (len1 != 4) && (len1 != 16)) ||
      ((len2 != 2) && (len2 != 4) && (len2 != 16))) {
    BTIF_TRACE_DEBUG("%s: invalid length", __func__);
    return false;
  }

  /* If lengths match, do a straight compare */
  if (len1 == len2) {
    if (len1 == 2)
      return ((p_uuid1[0] == p_uuid2[0]) && (p_uuid1[1] == p_uuid2[1]));
    if (len1 == 4)
      return ((p_uuid1[0] == p_uuid2[0]) && (p_uuid1[1] == p_uuid2[1]) &&
              (p_uuid1[2] == p_uuid2[2]) && (p_uuid1[3] == p_uuid2[3]));
    else
      return (memcmp(p_uuid1, p_uuid2, (size_t)len1) == 0);
  } else if (len1 > len2) {
    /* If the len1 was 4-byte, (so len2 is 2-byte), compare on the fly */
    if (len1 == 4) {
      return ((p_uuid1[0] == 0) && (p_uuid1[1] == 0) &&
              (p_uuid1[2] == p_uuid2[0]) && (p_uuid1[3] == p_uuid2[1]));
    } else {
      /* Normalize UUIDs to 16-byte form, then compare. Len1 must be 16 */
      memcpy(nu1, p_uuid1, Uuid::kNumBytes128);
      memcpy(nu2, sdp_base_uuid, Uuid::kNumBytes128);

      if (len2 == 4)
        memcpy(nu2, p_uuid2, len2);
      else if (len2 == 2)
        memcpy(nu2 + 2, p_uuid2, len2);

      return (memcmp(nu1, nu2, Uuid::kNumBytes128) == 0);
    }
  } else {
    /* len2 is greater than len1 */
    /* If the len2 was 4-byte, (so len1 is 2-byte), compare on the fly */
    if (len2 == 4) {
      return ((p_uuid2[0] == 0) && (p_uuid2[1] == 0) &&
              (p_uuid2[2] == p_uuid1[0]) && (p_uuid2[3] == p_uuid1[1]));
    } else {
      /* Normalize UUIDs to 16-byte form, then compare. Len1 must be 16 */
      memcpy(nu2, p_uuid2, Uuid::kNumBytes128);
      memcpy(nu1, sdp_base_uuid, Uuid::kNumBytes128);

      if (len1 == 4)
        memcpy(nu1, p_uuid1, (size_t)len1);
      else if (len1 == 2)
        memcpy(nu1 + 2, p_uuid1, (size_t)len1);

      return (memcmp(nu1, nu2, Uuid::kNumBytes128) == 0);
    }
  }
}

/*******************************************************************************
 *
 * Function         btdevice_set_current_tsep
 *
 * Description      set local stream end point(SEP) type: 0: source, 1: sink .
 *
 * Returns          none
 *
 ******************************************************************************/
void btdevice_set_current_tsep(uint8_t tsep) {
  btif_local_tsep = tsep;
  BTIF_TRACE_DEBUG("%s tsep %d", __func__, btif_local_tsep);
}

/*******************************************************************************
 *
 * Function         btdevice_get_current_tsep
 *
 * Description      get local stream end point(SEP) type.
 *
 * Returns          0: source, 1: sink
 *
 ******************************************************************************/
uint8_t btdevice_get_current_tsep(void) {
  BTIF_TRACE_DEBUG("%s tsep %d", __func__, btif_local_tsep);
  return btif_local_tsep;
}

/*******************************************************************************
 *
 * Function         btdevice_parse_services
 *
 * Description      parse services and set current tsep
 *
 * Returns          NONE
 *
 ******************************************************************************/
void btdevice_parse_services(void *uuids_req) {
  if (uuids_req == NULL) {
    return;
  }
  tSDP_UUID_SEQ *uuids_ptr = (tSDP_UUID_SEQ *)uuids_req;
  for (int yy = 0; yy < uuids_ptr->num_uids; yy++) {
    if (compare_uuid_arrays((uint8_t *)&src_uuid, 2,
                            &uuids_ptr->uuid_entry[yy].value[0],
                            uuids_ptr->uuid_entry[yy].len)) {
      btdevice_set_current_tsep(AVDT_TSEP_SRC);
      BTIF_TRACE_DEBUG("%s SOURCE !!!!!", __func__);
      break;
    } else if (compare_uuid_arrays((uint8_t *)&sink_uuid, 2,
                                   &uuids_ptr->uuid_entry[yy].value[0],
                                   uuids_ptr->uuid_entry[yy].len)) {
      btdevice_set_current_tsep(AVDT_TSEP_SNK);
      BTIF_TRACE_DEBUG("%s SINK !!!!!", __func__);
      break;
    }
  }
}

/*******************************************************************************
 *
 * Function         btdevice_get_device_type
 *
 * Description      get device of remote_bd_addr's type(A2dp Sink or A2dp Source).
 *
 * Returns          0: source device, 1: sink device
 *
 ******************************************************************************/
uint8_t btdevice_get_device_type(const RawAddress* remote_bd_addr) {
  bt_property_t remote_propertie;
  Uuid remote_uuids[BT_MAX_NUM_UUIDS];
  uint8_t devicetype = AVDT_TSEP_INVALID;
  /* REMOTE UUIDs */
  memset(remote_uuids, 0x00, sizeof(remote_uuids));
  BTDEVICE_STORAGE_GET_REMOTE_PROP(remote_bd_addr, BT_PROPERTY_UUIDS,
                                   remote_uuids, sizeof(remote_uuids),
                                   remote_propertie);
  int num_uuids = remote_propertie.len/sizeof(Uuid);

  BTIF_TRACE_DEBUG("%s num_uuids %d", __func__, num_uuids);
  //first check source device
  for (int yy = 0; yy < num_uuids; yy++) {
    uint16_t u16val = remote_uuids[yy].As16Bit();
    BTIF_TRACE_DEBUG("%s u16val 0x%x source 0x%x", __func__, u16val, UUID_SERVCLASS_AUDIO_SOURCE);
    if (u16val == UUID_SERVCLASS_AUDIO_SOURCE) {
      devicetype = AVDT_TSEP_SRC;
      BTIF_TRACE_DEBUG("%s SOURCE !!!!!", __func__);
    } else if (u16val == UUID_SERVCLASS_AUDIO_SINK) {
      devicetype = AVDT_TSEP_SNK;
      BTIF_TRACE_DEBUG("%s SINK !!!!!", __func__);
      break;
    }
  }
  return devicetype;
}

/*******************************************************************************
 *
 * Function         btdevice_avrcp_control_correct
 *
 * Description      if current is a2dp sink the  control must include BTA_AV_FEAT_RCCT.
 *                         if current is a2dp source,  control must not include BTA_AV_FEAT_RCCT.
 *
 * Returns          true: correct; false:not correct
 *
 ******************************************************************************/
bool btdevice_ccb_control_correct(uint8_t control) {
#ifdef HOST_DEVICE_COEXISTENCE
  bool ret = false;
  if ((control & BTA_AV_FEAT_RCCT)) {
    if ((btdevice_get_current_tsep() == AVDT_TSEP_SNK))
      ret = true;
  } else {
    if ((btdevice_get_current_tsep() == AVDT_TSEP_SRC))
      ret = true;
  }
  BTIF_TRACE_DEBUG("%s control 0x%x ret %d", __func__, control, ret);
  return ret;
#else
  return true;
#endif
}
