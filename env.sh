#/bin/bash
# apt install gcc-12-aarch64-linux-gnu g++-12-aarch64-linux-gnu sshpass
# apt install gcc-12-arm-linux-gnueabihf g++-12-arm-linux-gnueabihf sshpass
#apt install gcc-11-arm-linux-gnueabihf g++-11-arm-linux-gnueabihf
export AR=aarch64-linux-gnu-ar \
export CC=aarch64-linux-gnu-g++-12 \
export CXX=aarch64-linux-gnu-g++-12 \
export LINK=aarch64-linux-gnu-g++-12

# export AR=arm-linux-gnueabihf-ar \
# export CC=arm-linux-gnueabihf-gcc-12 \
# export CXX=arm-linux-gnueabihf-g++-12 \
# export LINK=arm-linux-gnueabihf-g++-12
echo "uploading files"
sshpass -p 'xiaoding' scp ./build/Release/camera.node  eric@192.168.137.25:/home/eric/picam/build/Release
sshpass -p 'xiaoding' scp ./src/index.js  eric@192.168.137.25:/home/eric/picam/src/index.js
sshpass -p 'xiaoding' scp ./src/camera.js  eric@192.168.137.25:/home/eric/picam/src/camera.js
# sshpass -p 'xiaoding' scp -r ./zfec  eric@192.168.137.56:/home/eric/picam/zfec
# sshpass -p 'xiaoding' scp ./package.json  eric@192.168.137.25:/home/eric/picam/package.json
# sshpass -p 'xiaoding' scp ./binding.gyp  eric@192.168.137.25:/home/eric/picam/binding.gyp
# sshpass -p 'xiaoding' scp -r ./camera  eric@192.168.137.25:/home/eric/picam/
# sshpass -p 'xiaoding' scp -r ./include  eric@192.168.137.25:/home/eric/picam/
# vcgencmd measure_clock h264
# vcgencmd get_mem gpu
# vcgencmd get_mem arm
# vcgencmd measure_temp