#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b main https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard 6880cddec1a48c018511820929ad9d11ae23105a
./dnn/download_model.sh "735117b"
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
