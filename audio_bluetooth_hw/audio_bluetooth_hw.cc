/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "BTAudioHw"

#include <android-base/logging.h>
#include <errno.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <malloc.h>
#include <string.h>
#include <system/audio.h>

#include "stream_apis.h"
#include "utils.h"

#ifdef SPRD_FEATURE_AOBFIX
extern struct audio_stream_out* stream_ptr;
using ::android::bluetooth::audio::utils::ParseAudioParams;
using ::android::bluetooth::audio::utils::GetAudioParamString;

static int adev_sprd_set_parameters(struct audio_hw_device* dev,
                               const char* kvpairs) {
  LOG(INFO) << __func__;
  if (!stream_ptr) {
    LOG(INFO) << __func__ << ": stream_ptr is NULL, return.";
    return -1;
  }
  auto* out = reinterpret_cast<BluetoothStreamOut*>(stream_ptr);
  std::unique_lock<std::mutex> lock(out->mutex_);

  LOG(INFO) << __func__ << ": state=" << out->bluetooth_output_.GetState()
               << ", kvpairs=[" << kvpairs << "]";

  std::unordered_map<std::string, std::string> params =
      ParseAudioParams(kvpairs);
  if (params.empty()) return -1;

  LOG(INFO) << __func__ << ": ParamsMap=[" << GetAudioParamString(params)
               << "]";

  if (params.find("A2dpSuspended") != params.end()) {
    if (params["A2dpSuspended"] == "true") {
      LOG(INFO) << __func__ << ": state=" << out->bluetooth_output_.GetState()
                << " stream param stopped";
      if (out->bluetooth_output_.GetState() != BluetoothStreamState::DISABLED) {
        out->frames_rendered_ = 0;
        out->bluetooth_output_.Stop();
      }
    } else {
      LOG(INFO) << __func__ << ": state=" << out->bluetooth_output_.GetState()
                << " stream param standby";
      if (out->bluetooth_output_.GetState() == BluetoothStreamState::DISABLED) {
        out->bluetooth_output_.SetState(BluetoothStreamState::STANDBY);
      }
    }
  }
  LOG(INFO) << __func__ << ": state=" << out->bluetooth_output_.GetState()
               << ", kvpairs=[" << kvpairs << "]";
  return 0;
}
#else
static int adev_set_parameters(struct audio_hw_device* dev,
                               const char* kvpairs) {
  LOG(INFO) << __func__ << ": kevpairs=[" << kvpairs << "]";
  return -ENOSYS;
}
#endif

static char* adev_get_parameters(const struct audio_hw_device* dev,
                                 const char* keys) {
  LOG(INFO) << __func__ << ": keys=[" << keys << "]";
  return strdup("");
}

static int adev_init_check(const struct audio_hw_device* dev) { return 0; }

static int adev_set_voice_volume(struct audio_hw_device* dev, float volume) {
  LOG(VERBOSE) << __func__ << ": volume=" << volume;
  return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device* dev, float volume) {
  LOG(VERBOSE) << __func__ << ": volume=" << volume;
  return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device* dev, float* volume) {
  return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device* dev, bool muted) {
  LOG(VERBOSE) << __func__ << ": mute=" << muted;
  return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device* dev, bool* muted) {
  return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device* dev, audio_mode_t mode) {
  LOG(VERBOSE) << __func__ << ": mode=" << mode;
  return 0;
}

static int adev_set_mic_mute(struct audio_hw_device* dev, bool state) {
  LOG(VERBOSE) << __func__ << ": state=" << state;
  return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device* dev, bool* state) {
  return -ENOSYS;
}

static int adev_dump(const audio_hw_device_t* device, int fd) { return 0; }

static int adev_close(hw_device_t* device) {
#ifdef SPRD_FEATURE_AOBFIX  
  LOG(INFO) << __func__ << "close " ;
#endif
  free(device);
  return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device) {
  LOG(INFO) << __func__ << ": name=[" << name << "]";
  if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) return -EINVAL;

  struct audio_hw_device* adev =
      (struct audio_hw_device*)calloc(1, sizeof(struct audio_hw_device));
  if (!adev) return -ENOMEM;

  adev->common.tag = HARDWARE_DEVICE_TAG;
  adev->common.version = AUDIO_DEVICE_API_VERSION_2_0;
  adev->common.module = (struct hw_module_t*)module;
  adev->common.close = adev_close;

  adev->init_check = adev_init_check;
  adev->set_voice_volume = adev_set_voice_volume;
  adev->set_master_volume = adev_set_master_volume;
  adev->get_master_volume = adev_get_master_volume;
  adev->set_mode = adev_set_mode;
  adev->set_mic_mute = adev_set_mic_mute;
  adev->get_mic_mute = adev_get_mic_mute;
#ifdef SPRD_FEATURE_AOBFIX
  adev->set_parameters = adev_sprd_set_parameters;
#else
  adev->set_parameters = adev_set_parameters;
#endif
  adev->get_parameters = adev_get_parameters;
  adev->get_input_buffer_size = adev_get_input_buffer_size;
  adev->open_output_stream = adev_open_output_stream;
  adev->close_output_stream = adev_close_output_stream;
  adev->open_input_stream = adev_open_input_stream;
  adev->close_input_stream = adev_close_input_stream;
  adev->dump = adev_dump;
  adev->set_master_mute = adev_set_master_mute;
  adev->get_master_mute = adev_get_master_mute;

  *device = &adev->common;
  return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = AUDIO_HARDWARE_MODULE_ID,
            .name = "Bluetooth Audio HW HAL",
            .author = "The Android Open Source Project",
            .methods = &hal_module_methods,
        },
};
