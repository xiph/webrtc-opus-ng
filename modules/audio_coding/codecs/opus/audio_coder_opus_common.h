/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_CODER_OPUS_COMMON_H_
#define MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_CODER_OPUS_COMMON_H_

#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "api/audio_codecs/audio_decoder.h"
#include "api/audio_codecs/audio_format.h"
#include "rtc_base/string_to_number.h"
#include "rtc_base/copy_on_write_buffer.h"

namespace webrtc {

absl::optional<std::string> GetFormatParameter(const SdpAudioFormat& format,
                                               absl::string_view param);

template <typename T>
absl::optional<T> GetFormatParameter(const SdpAudioFormat& format,
                                     absl::string_view param) {
  return rtc::StringToNumber<T>(GetFormatParameter(format, param).value_or(""));
}

template <>
absl::optional<std::vector<unsigned char>> GetFormatParameter(
    const SdpAudioFormat& format,
    absl::string_view param);

class OpusFrame : public AudioDecoder::EncodedAudioFrame {
 public:
  OpusFrame(AudioDecoder* decoder,
            rtc::Buffer&& payload,
            bool is_primary_payload)
      : decoder_(decoder),
        payload_(std::move(payload)),
        is_primary_payload_(is_primary_payload),
        dred_index_(0),
        dred_primary_timestamp_(0) {}

  OpusFrame(AudioDecoder* decoder,
            const rtc::CopyOnWriteBuffer& dred_payload,
            int dred_index,
            uint32_t dred_primary_timestamp)
      : decoder_(decoder),
        is_primary_payload_(false),
        dred_payload_(dred_payload),
        dred_index_(dred_index),
        dred_primary_timestamp_(dred_primary_timestamp) {}

  size_t Duration() const override {
    int ret;
    if (dred_index_ > 0) {
      // by convention DRED packets are always 10 msec.
      return decoder_->SampleRateHz() / 100;
    }
    if (is_primary_payload_) {
      ret = decoder_->PacketDuration(payload_.data(), payload_.size());
    } else {
      ret = decoder_->PacketDurationRedundant(payload_.data(), payload_.size());
    }
    return (ret < 0) ? 0 : static_cast<size_t>(ret);
  }

  bool IsDtxPacket() const override { return payload_.size() <= 2 && dred_index_ == 0; }

  absl::optional<DecodeResult> Decode(
      rtc::ArrayView<int16_t> decoded) const override {
    AudioDecoder::SpeechType speech_type = AudioDecoder::kSpeech;
    int ret;
    if (is_primary_payload_) {
      ret = decoder_->Decode(
          payload_.data(), payload_.size(), decoder_->SampleRateHz(),
          decoded.size() * sizeof(int16_t), decoded.data(), &speech_type);
    } else if (dred_index_ > 0 && dred_payload_.size() > 0) {
      ret = decoder_->DecodeDred(dred_payload_.data(), dred_payload_.size(), dred_primary_timestamp_, decoded.data(), dred_index_);
    } else {
      ret = decoder_->DecodeRedundant(
          payload_.data(), payload_.size(), decoder_->SampleRateHz(),
          decoded.size() * sizeof(int16_t), decoded.data(), &speech_type);
    }

    if (ret < 0)
      return absl::nullopt;

    return DecodeResult{static_cast<size_t>(ret), speech_type};
  }

 private:
  AudioDecoder* const decoder_;
  const rtc::Buffer payload_;
  const bool is_primary_payload_;
  // TODO(klingm@amazon.com) consider using use a single copy on write buffer for both payload and/or DRED
  const rtc::CopyOnWriteBuffer dred_payload_;
  const int dred_index_;
  const uint32_t dred_primary_timestamp_;
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_CODER_OPUS_COMMON_H_
