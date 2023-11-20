/*
 *  Copyright (c) 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtpplay_utils.h"
#include "rtc_base/byte_order.h"

namespace webrtc {
namespace test {

bool read_packet(RD_packet_t &packet_header, uint8_t *buf, size_t *packet_len, FILE *input_file)
{
    if (fread(&packet_header.length, sizeof(packet_header.length), 1, input_file) != 1)
        return false;
    if (fread(&packet_header.plen, sizeof(packet_header.plen), 1, input_file) != 1)
        return false;
    if (fread(&packet_header.offset, sizeof(packet_header.offset), 1, input_file) != 1)
        return false;

    packet_header.length = be16toh(packet_header.length);
    packet_header.plen = be16toh(packet_header.plen);
    packet_header.offset = be32toh(packet_header.offset);

    // Note - packet_header.length is the true size of this entry in the file
    // and includes 8 bytes for the packet_header itself.
    // packet_header.plen is the original length of the RTP packet and could be
    // different in the case where truncated data was written to the file
    // (such as from a converted pcap). For our use case, we only care about what
    // is actually in the file, so plen is ignored.
    size_t packet_length = packet_header.length;
    if (packet_length < kRtpPlayPacketHeaderSize) {
        // length too short
        return false;
    }
    // subtract off header length to get true packet size
    packet_length -= kRtpPlayPacketHeaderSize;
    if (packet_length > kMaxRtpPlayBufferSize) {
        // length too big
        return false;
    }
    if (fread(buf, sizeof(uint8_t), packet_length, input_file) != packet_length) {
        return false;
    }
    *packet_len = packet_length;
    return true;
}

bool write_packet(RD_packet_t &packet_header, const uint8_t *buf, size_t buf_size, FILE *output_file)
{
    packet_header.length = htobe16(packet_header.length);
    packet_header.plen = htobe16(packet_header.plen);
    packet_header.offset = htobe32(packet_header.offset);

    if (fwrite(&packet_header.length, sizeof(packet_header.length), 1, output_file) != 1)
        return false;
    if (fwrite(&packet_header.plen, sizeof(packet_header.plen), 1, output_file) != 1)
        return false;
    if (fwrite(&packet_header.offset, sizeof(packet_header.offset), 1, output_file) != 1)
        return false;
    if (fwrite(buf, sizeof(uint8_t), buf_size, output_file) != buf_size)
        return false;

    return true;
}

}  // namespace test
}  // namespace webrtc
