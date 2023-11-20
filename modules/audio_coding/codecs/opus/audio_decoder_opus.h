/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_DECODER_OPUS_H_
#define MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_DECODER_OPUS_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "api/audio_codecs/audio_decoder.h"
#include "modules/audio_coding/codecs/opus/opus_interface.h"
#include "rtc_base/buffer.h"

namespace webrtc {

class AudioDecoderOpusImpl final : public AudioDecoder {
 public:
  explicit AudioDecoderOpusImpl(size_t num_channels,
                                int sample_rate_hz = 48000);
  ~AudioDecoderOpusImpl() override;

  AudioDecoderOpusImpl(const AudioDecoderOpusImpl&) = delete;
  AudioDecoderOpusImpl& operator=(const AudioDecoderOpusImpl&) = delete;

  std::vector<ParseResult> ParsePayload(rtc::Buffer&& payload,
                                        uint32_t timestamp) override;
  std::vector<ParseResult> ParsePayloadRedundancy(rtc::Buffer&& payload,
                                                  uint32_t timestamp,
                                                  uint32_t recovery_timestamp_offset) override;

  void GeneratePlc(size_t requested_samples_per_channel,
                   rtc::BufferT<int16_t>* concealment_audio) override;

  void Reset() override;
  int PacketDuration(const uint8_t* encoded, size_t encoded_len) const override;
  int PacketDurationRedundant(const uint8_t* encoded,
                              size_t encoded_len) const override;
  bool PacketHasFec(const uint8_t* encoded, size_t encoded_len) const override;
  int SampleRateHz() const override;
  size_t Channels() const override;

 protected:
  int DecodeInternal(const uint8_t* encoded,
                     size_t encoded_len,
                     int sample_rate_hz,
                     int16_t* decoded,
                     SpeechType* speech_type) override;
  int DecodeRedundantInternal(const uint8_t* encoded,
                              size_t encoded_len,
                              int sample_rate_hz,
                              int16_t* decoded,
                              SpeechType* speech_type) override;

  int DecodeDredInternal(const uint8_t *encoded,
                         size_t encoded_len,
                         uint32_t primary_timestamp,
                         int16_t* decoded,
                         int index) override;
 private:
  OpusDecInst* dec_state_;
  const size_t channels_;
  const int sample_rate_hz_;
  uint32_t last_decoded_dred_timestamp_;
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_CODING_CODECS_OPUS_AUDIO_DECODER_OPUS_H_
