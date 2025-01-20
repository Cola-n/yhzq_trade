#!/bin/bash

# 获取当前日期，格式为 YYYYMMDD
current_date=$(date +"%Y%m%d")

# 构建日志文件夹路径
log_dir="/root/niqin_144/yhzq_trade/log/$current_date"
log_file="$log_dir/outlog.log"

# 检查日志文件夹是否存在，如果不存在则创建它
if [ ! -d "$log_dir" ]; then
    mkdir -p "$log_dir"
fi

# 定义 build 文件夹路径
BUILD_DIR="/root/niqin_144/yhzq_trade/build"

# 清理 build 文件夹
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning up $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
else
    echo "$BUILD_DIR does not exist. Creating it..."
fi

# 创建 build 文件夹
mkdir "$BUILD_DIR"
cd "$BUILD_DIR" || exit

# 执行 cmake
echo "Running cmake..."
cmake ../demo/simple_demo

# 执行 make
echo "Running make..."
make

# 执行测试程序
export LD_LIBRARY_PATH=/root/niqin_144/yhzq_trade/lib:$LD_LIBRARY_PATH
export RSA_PUBLIC_KEY_PATH=/root/niqin_144/yhzq_trade/lib/rsa_public_key.pem
export ATP_LOGIN_ENCRYPT_PASSWORD=/root/niqin_144/yhzq_trade/lib/librsa_2048_encrypt.so
echo $ATP_LOGIN_ENCRYPT_PASSWORD

echo "Running ./trade..."
./simple_trade_api_demo | tee -a "$log_file"
