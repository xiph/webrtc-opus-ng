#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b opus-ng https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard b75bd48d82281193681c49c00fca9773a45fb0d8
./dnn/download_model.sh "735117b"
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
