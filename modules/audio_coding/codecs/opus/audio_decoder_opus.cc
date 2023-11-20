/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_coding/codecs/opus/audio_decoder_opus.h"

#include <memory>
#include <utility>

#include "absl/types/optional.h"
#include "api/array_view.h"
#include "modules/audio_coding/codecs/opus/audio_coder_opus_common.h"
#include "rtc_base/checks.h"

namespace webrtc {

AudioDecoderOpusImpl::AudioDecoderOpusImpl(size_t num_channels,
                                           int sample_rate_hz)
    : channels_{num_channels}, sample_rate_hz_{sample_rate_hz} {
  RTC_DCHECK(num_channels == 1 || num_channels == 2);
  RTC_DCHECK(sample_rate_hz == 16000 || sample_rate_hz == 48000);
  const int error =
      WebRtcOpus_DecoderCreate(&dec_state_, channels_, sample_rate_hz_);
  RTC_DCHECK(error == 0);
  WebRtcOpus_DecoderInit(dec_state_);
}

AudioDecoderOpusImpl::~AudioDecoderOpusImpl() {
  WebRtcOpus_DecoderFree(dec_state_);
}

std::vector<AudioDecoder::ParseResult> AudioDecoderOpusImpl::ParsePayload(
    rtc::Buffer&& payload,
    uint32_t timestamp) {
  std::vector<ParseResult> results;

  if (PacketHasFec(payload.data(), payload.size())) {
    const int duration =
        PacketDurationRedundant(payload.data(), payload.size());
    RTC_DCHECK_GE(duration, 0);
    rtc::Buffer payload_copy(payload.data(), payload.size());
    std::unique_ptr<EncodedAudioFrame> fec_frame(
        new OpusFrame(this, std::move(payload_copy), false));
    results.emplace_back(timestamp - duration, 1, std::move(fec_frame));
  }
  std::unique_ptr<EncodedAudioFrame> frame(
      new OpusFrame(this, std::move(payload), true));
  results.emplace_back(timestamp, 0, std::move(frame));
  return results;
}

std::vector<AudioDecoder::ParseResult> AudioDecoderOpusImpl::ParsePayloadRedundancy(
    rtc::Buffer&& payload,
    uint32_t timestamp,
    uint32_t recovery_timestamp_offset) {
  std::vector<ParseResult> results;
  uint32_t begin_timestamp = timestamp;
  if (PacketHasFec(payload.data(), payload.size())) {
    const int duration =
        PacketDurationRedundant(payload.data(), payload.size());
    RTC_DCHECK_GE(duration, 0);
    // TODO(klingm@amazon.com) - eliminate this copy with a shared read-only buffer
    rtc::Buffer payload_copy(payload.data(), payload.size());
    std::unique_ptr<EncodedAudioFrame> fec_frame(
        new OpusFrame(this, std::move(payload_copy), false));
    results.emplace_back(timestamp - duration, 1, std::move(fec_frame));
    begin_timestamp -= duration;
  }
  if (recovery_timestamp_offset > 0 ) {
    rtc::CopyOnWriteBuffer dred_data(opus_dred_get_size());
    int samps = WebRtcOpus_DredParse(dec_state_,
                         dred_data.MutableData(),
                         payload.data(),
                         payload.size(),
                         recovery_timestamp_offset);
    // find the desired number of 10 msec. chunks
    int dred_count = 100*samps/sample_rate_hz_;
    int desired_dred_count = 100*recovery_timestamp_offset/sample_rate_hz_;
    // printf("got %d chunks of dred, offset %d, recovery_timestamp_offset %u\n", dred_count, desired_dred_count, recovery_timestamp_offset);
    if (dred_count > 0 && desired_dred_count > 0) {
      if (dred_count > desired_dred_count) dred_count = desired_dred_count;
      last_decoded_dred_timestamp_ = timestamp;
      uint32_t recovery_timestamp = timestamp - dred_count*sample_rate_hz_/100;
      for (int i = 0; i < dred_count; ++i) {
        // make sure recovery_timestamp < begin_timestamp
        if ((begin_timestamp == recovery_timestamp) ||
            (begin_timestamp - recovery_timestamp) >= 0xFFFFFFFF / 2)
          break;
        auto p = new OpusFrame(this, dred_data, dred_count - i, timestamp);
        std::unique_ptr<EncodedAudioFrame> frame(p);
        // Deep REDundancy packets have a priority of 2
        // (LBRR FEC with a priority of 1 will be preferred, if available)
        results.emplace_back(recovery_timestamp, 2, std::move(frame));
        recovery_timestamp += sample_rate_hz_/100;
      }
    }
  }
  std::unique_ptr<EncodedAudioFrame> frame(
      new OpusFrame(this, std::move(payload), true));
  results.emplace_back(timestamp, 0, std::move(frame));
  return results;
}

void AudioDecoderOpusImpl::GeneratePlc(size_t requested_samples_per_channel,
                                       rtc::BufferT<int16_t>* concealment_audio)
{
#if WEBRTC_OPUS_USE_CODEC_PLC
  concealment_audio->SetSize(PacketDuration(NULL, 0) * channels_);
  int16_t temp_type = 1;
  int ret = WebRtcOpus_Decode(dec_state_, NULL, 0, concealment_audio->data(), &temp_type);
  if (ret > 0) {
    ret *= static_cast<int>(channels_);
    concealment_audio->SetSize(ret);
  } else {
    concealment_audio->SetSize(0);
  }
#endif
}

int AudioDecoderOpusImpl::DecodeInternal(const uint8_t* encoded,
                                         size_t encoded_len,
                                         int sample_rate_hz,
                                         int16_t* decoded,
                                         SpeechType* speech_type) {
  RTC_DCHECK_EQ(sample_rate_hz, sample_rate_hz_);
  int16_t temp_type = 1;  // Default is speech.
  int ret =
      WebRtcOpus_Decode(dec_state_, encoded, encoded_len, decoded, &temp_type);
  if (ret > 0)
    ret *= static_cast<int>(channels_);  // Return total number of samples.
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderOpusImpl::DecodeRedundantInternal(const uint8_t* encoded,
                                                  size_t encoded_len,
                                                  int sample_rate_hz,
                                                  int16_t* decoded,
                                                  SpeechType* speech_type) {
  if (!PacketHasFec(encoded, encoded_len)) {
    // This packet is a RED packet.
    return DecodeInternal(encoded, encoded_len, sample_rate_hz, decoded,
                          speech_type);
  }

  RTC_DCHECK_EQ(sample_rate_hz, sample_rate_hz_);
  int16_t temp_type = 1;  // Default is speech.
  int ret = WebRtcOpus_DecodeFec(dec_state_, encoded, encoded_len, decoded,
                                 &temp_type);
  if (ret > 0)
    ret *= static_cast<int>(channels_);  // Return total number of samples.
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderOpusImpl::DecodeDredInternal(const uint8_t *encoded,
                                             size_t encoded_len,
                                             uint32_t primary_timestamp,
                                             int16_t* decoded,
                                             int index) {
  int ret = WebRtcOpus_DecodeDred(dec_state_, encoded, decoded, index);
  if (ret > 0)
    ret *= static_cast<int>(channels_);  // Return total number of samples.
  return ret;
}

void AudioDecoderOpusImpl::Reset() {
  WebRtcOpus_DecoderInit(dec_state_);
}

int AudioDecoderOpusImpl::PacketDuration(const uint8_t* encoded,
                                         size_t encoded_len) const {
  return WebRtcOpus_DurationEst(dec_state_, encoded, encoded_len);
}

int AudioDecoderOpusImpl::PacketDurationRedundant(const uint8_t* encoded,
                                                  size_t encoded_len) const {
  if (!PacketHasFec(encoded, encoded_len)) {
    // This packet is a RED packet.
    return PacketDuration(encoded, encoded_len);
  }

  return WebRtcOpus_FecDurationEst(encoded, encoded_len, sample_rate_hz_);
}

bool AudioDecoderOpusImpl::PacketHasFec(const uint8_t* encoded,
                                        size_t encoded_len) const {
  int fec;
  fec = WebRtcOpus_PacketHasFec(encoded, encoded_len);
  return (fec == 1);
}

int AudioDecoderOpusImpl::SampleRateHz() const {
  return sample_rate_hz_;
}

size_t AudioDecoderOpusImpl::Channels() const {
  return channels_;
}

}  // namespace webrtc
