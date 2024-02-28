# WebRTC and Next Generation Opus Audio Codec



## Before You Start
If needed, review the [WebRTC development guide](https://webrtc.github.io/webrtc-org/native-code/development/) for general information on building libWebRTC.

Be sure to install the necessary [prerequisite software](https://webrtc.github.io/webrtc-org/native-code/development/).


## Building
```
mkdir webrtc-opus-ng
cd webrtc-opus-ng
gclient config --unmanaged --name=src https://github.com/xiph/webrtc-opus-ng.git
gclient sync -v -r opus-ng
./src/opus_ng/setup.sh
```
### Debug Build
```
cd src
gn gen ../out/Debug --args="opus_codec_support_dred=true rtc_opus_support_dred=true rtc_opus_use_codec_plc=true"
cd ..
ninja -v -C ./out/Debug
```

### Release Build
```
cd src
gn gen ../out/Release --args="opus_codec_support_dred=true rtc_opus_support_dred=true rtc_opus_use_codec_plc=true is_debug=false"
cd ..
ninja -v -C ./out/Release
```
### iOS Debug Build
```
cd src
gn gen ../out/Debug-ios --args="opus_codec_support_dred=true rtc_opus_support_dred=true rtc_opus_use_codec_plc=true target_os=\"ios\" ios_enable_code_signing=false rtc_build_tools=false rtc_include_tests=false target_environment=\"device\" target_cpu=\"arm64\""
cd ..
ninja -v -C ./out/Release
```
## Try it Out

Note: `rtp_encode` does not seem to produce any output when run in release mode, so use the debug build. For this example, decoding with DRED runs about 20x faster in release mode vs. debug (on x86_64)

### Neural PLC
```
./out/Debug/rtp_encode ./src/resources/speech_and_misc_wb.pcm speech_and_misc_nodred.rtp --sample_rate 16000 --frame_len 20 --codec opus --fec=true --expected_loss 20
./out/Release/rtp_apply_loss speech_and_misc_nodred.rtp speech_and_misc_nodred_loss.rtp --loss_file ./src/opus_ng/high_is_lost.txt
./out/Release/neteq_rtpplay speech_and_misc_nodred_loss.rtp speech_and_misc_nodred_loss.wav
```

### Deep Redundancy (DRED)
Adding 1 second of redundancy.
```
./out/Debug/rtp_encode ./src/resources/speech_and_misc_wb.pcm speech_and_misc_dred.rtp --sample_rate 16000 --frame_len 20 --codec opus --fec=true --expected_loss 20 --dred 100
./out/Release/rtp_apply_loss speech_and_misc_dred.rtp speech_and_misc_dred_loss.rtp --loss_file ./src/opus_ng/high_is_lost.txt
./out/Release/neteq_rtpplay speech_and_misc_dred_loss.rtp speech_and_misc_dred_loss.wav

```