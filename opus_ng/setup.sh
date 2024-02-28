#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b main https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard 23c591318e63f9f38a2d60b361230f148e29fb70
./dnn/download_model.sh "735117b"
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
