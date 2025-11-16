# node-libcamera
nodejs binding for libcamera

## 1. Install applications (Crossplatform)
sudo apt install libcamera-dev libtiff-dev libexif-dev libturbojpeg0-dev 

* #### compile for Raspberry Pi OS 64 bit system (Bookworm)   
    sudo apt install clang gcc-12-aarch64-linux-gnu g++-12-aarch64-linux-gnu  
* #### compile for Raspberry Pi OS 32 bit system (Bookworm)   
    sudo apt install clang gcc-12-arm-linux-gnueabihf g++-12-arm-linux-gnueabihf   

## 2. Fetch .so files (Crossplatform)
fetch 
```
lib64/libcamera.so.0.5 
libcamera-base.so.0.5.1 
libexif.so.12.3.4 
libjpeg.so.62.3.0 
libtiff.so.6.0.0 
libturbojpeg.so.0.2.0
```
from Raspberry Pi to project workspace
## 3. Configure
npm run configure
## 4. Build Binary
npm run build
## 5. Build Lib
npm run esbuild