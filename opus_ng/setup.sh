#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b main https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard 57901a6758c3bdc7481d61669812bde13d2085b8
./dnn/download_model.sh "735117b"
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
