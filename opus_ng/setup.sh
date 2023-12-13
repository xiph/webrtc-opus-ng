#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b opus-ng https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard 6d7ae213ce07cd3090a3d77ad0e79035bcf5d191
./dnn/download_model.sh df63771
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
