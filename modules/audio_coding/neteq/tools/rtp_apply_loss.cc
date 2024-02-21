/*
 *  Copyright (c) 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>
#include <random>

#include "modules/rtp_rtcp/source/rtp_packet.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include "rtpplay_utils.h"

ABSL_FLAG(float,
          loss,
          0,
          "Packet loss percentage");

ABSL_FLAG(std::string,
          loss_file,
          "",
          "Text file indicating which packets are lost - 1 packet per line, 0 indicates no loss, 1 indicates loss");

ABSL_FLAG(unsigned int,
          seed,
          0,
          "Unsigned integer seed value for random number generator - if 0 use /dev/random");

namespace webrtc {
namespace test {
namespace {

int apply_loss(int argc, char **argv) {
    absl::SetProgramUsageMessage("This tool takes an input file in rtpplay format, applies packet "
        "loss, and writes the output to a new rtpplay file.\n\n"
        "usage - rtp_apply_loss <input.rtp> <output.rtp> --loss <percent> --loss_file <loss.txt> --seed <unsigned int>");
    std::vector<char*> args = absl::ParseCommandLine(argc, argv);
    if (argc < 3) {
        fwrite(absl::ProgramUsageMessage().data(), absl::ProgramUsageMessage().size(), 1, stderr);
        fprintf(stderr, "\n\nfor help - rtp_apply_loss --help\n\n");
        return -1;
    }
    const char *input_name, *output_name;
    input_name = argv[1];
    output_name = argv[2];
    const float loss_percent = absl::GetFlag(FLAGS_loss);
    const std::string loss_file_name(absl::GetFlag(FLAGS_loss_file));
    unsigned int random_seed = absl::GetFlag(FLAGS_seed);

    FILE *input_file = fopen(input_name, "r");
    if (!input_file) {
        fprintf(stderr, "Failed to open %s\n", input_name);
        return -1;
    }
    FILE *output_file = fopen(output_name, "w");
    if (!output_file) {
        fprintf(stderr, "Failed to open %s\n", output_name);
        return -1;
    }
    FILE *loss_file = nullptr;
    if (loss_file_name.size() > 0) {
        loss_file = fopen(loss_file_name.c_str(), "r");
        if (!loss_file) {
            fprintf(stderr, "Failed to open %s\n", loss_file_name.c_str());
            return -1;
        }
    }

    uint8_t buf[kMaxRtpPlayBufferSize];
    // read rtpplay line
    if (!fgets((char *)buf, kMaxRtpPlayBufferSize, input_file)) {
        fprintf(stderr, "Failed to read first line of input rtpplay file %s\n", input_name);
        return -1;
    }
    // write rtpplay line
    if (fwrite(buf, strlen((const char *)buf), 1, output_file) != 1) {
        fprintf(stderr, "Failed to write first line of output rtpplay file %s\n", output_name);
        return -1;
    }
    // read 16-byte RD_hdr_t
    if (fread(buf, kRtpPlayFileHeaderSize, 1, input_file) != 1) {
        fprintf(stderr, "Failed to read header of input rtpplay file %s\n", input_name);
        return -1;
    }
    // write 16-byte RD_hdr_t
    if (fwrite(buf, kRtpPlayFileHeaderSize, 1, output_file) != 1) {
        fprintf(stderr, "Failed to write header of output rtpplay file %s\n", output_name);
        return -1;
    }
    std::unique_ptr<std::mt19937> rng;
    std::uniform_real_distribution<float> random_pct(0.0, 100.0); // distribution in range [0, 100)
    if (!loss_file) {
        rng = std::make_unique<std::mt19937>();
        if (random_seed == 0) {
            std::random_device dev;
            random_seed = dev();
        }
        rng->seed(random_seed);
    }

    int count = 0;
    int writes = 0;
    while (1) {
        RD_packet_t packet_header;
        size_t packet_length;
        if (!read_packet(packet_header, buf, &packet_length, input_file))
            break;

        bool should_write = true;
        if (loss_file) {
            char linebuf[16];
            if (fgets(linebuf, sizeof(linebuf), loss_file)) {
                char *p = linebuf;
                while (*p && isspace(*p)) p++;
                should_write = *p == '0';
            } else {
                fprintf(stderr, "Warning - no loss information in loss file for packet %d\n", count);
            }
        } else if (loss_percent > 0 && random_pct(*rng) < loss_percent) {
            should_write = false;
        }
        if (should_write) {
            if (!write_packet(packet_header, buf, packet_length, output_file))
                break;
            ++writes;
        }
        ++count;
    }
    printf("Successfully processed %d packets, wrote %d packets to output\n", count, writes);

    fclose(input_file);
    fclose(output_file);
    if (loss_file) fclose(loss_file);

    return 0;
}

}  // namespace
}  // namespace test
}  // namespace webrtc

int main(int argc, char **argv) {
    return webrtc::test::apply_loss(argc, argv);
}
