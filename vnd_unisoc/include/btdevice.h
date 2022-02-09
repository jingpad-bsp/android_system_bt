/******************************************************************************
 *
 *  Copyright (C) 2019 Spreadtrum Communication Inc.
 *
 ******************************************************************************/

#ifndef BTDEVICE_H
#define BTDEVICE_H

#include "bt_types.h" /* This must be defined AFTER buildcfg.h */
#include <hardware/bt_av.h>
#include <raw_address.h>
/*******************************************************************************
 *
 * Function         btdevice_set_current_tsep
 *
 * Description      set local stream end point(SEP) type: 0: source, 1: sink .
 *
 * Returns          none
 *
 ******************************************************************************/
void btdevice_set_current_tsep(uint8_t tsep);

/*******************************************************************************
 *
 * Function         btdevice_get_current_tsep
 *
 * Description      get local stream end point(SEP) type .
 *
 * Returns          0: source, 1: sink
 *
 ******************************************************************************/
uint8_t btdevice_get_current_tsep();

/*******************************************************************************
 *
 * Function         btdevice_parse_services
 *
 * Description      parse services and set current tsep
 *
 * Returns          NONE
 *
 ******************************************************************************/
void btdevice_parse_services(void *uuids_req);

/*******************************************************************************
 *
 * Function         btdevice_get_device_type
 *
 * Description      get device of remote_bd_addr's type(A2dp Sink or A2dp Source).
 *
 * Returns          0: source device, 1: sink device
 *
 ******************************************************************************/
uint8_t btdevice_get_device_type(const RawAddress* remote_bd_addr);

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
bool  btdevice_ccb_control_correct(uint8_t control);
#endif /* BTDEVICE_H */
