/*
 *  Copyright (c) 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_CODING_NETEQ_TOOLS_RTPPLAY_UTILS_H_
#define MODULES_AUDIO_CODING_NETEQ_TOOLS_RTPPLAY_UTILS_H_

#include <stdio.h>
#include <cstdint>

namespace webrtc {
namespace test {

typedef struct {
    uint16_t length;   /* length of packet, including this header (may
                          be smaller than plen if not whole packet recorded) */
    uint16_t plen;     /* actual header+payload length for RTP, 0 for RTCP */
    uint32_t offset;   /* milliseconds since the start of recording */
} RD_packet_t;

bool read_packet(RD_packet_t& packet_header, uint8_t *buf, size_t *packet_len, FILE *input_file);

bool write_packet(RD_packet_t &packet_header, const uint8_t *buf, size_t buf_size, FILE *output_file);

static constexpr int kMaxRtpPlayBufferSize = 2048;
static constexpr int kRtpPlayFileHeaderSize = 16;
static constexpr int kRtpPlayPacketHeaderSize = 8;

}  // namespace test
}  // namespace webrtc
#endif // MODULES_AUDIO_CODING_NETEQ_TOOLS_RTPPLAY_UTILS_H_
