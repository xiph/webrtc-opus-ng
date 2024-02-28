#!/usr/bin/env bash
set -ex

pushd ./
cd ./src/opus_ng/opus
rm -rf src
git clone -b main https://gitlab.xiph.org/xiph/opus.git src
cd ./src
git reset --hard dcce2fd455b1f407bf4e1347ce5358a6d0096bd2
./dnn/download_model.sh "735117b"
popd
rm -rf ./src/third_party/opus
ln -snf ../opus_ng/opus ./src/third_party/opus
