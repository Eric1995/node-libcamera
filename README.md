# node-libcamera
nodejs binding for libcamera

## 1. Install applications (Crossplatform)
sudo apt install libcamera-dev libtiff-dev libexif-dev sshpass  

* #### compile for 64 bit system    
    sudo apt install gcc-12-aarch64-linux-gnu g++-12-aarch64-linux-gnu  
* #### compile for 32 bit system    
    sudo apt install gcc-12-arm-linux-gnueabihf g++-12-arm-linux-gnueabihf   

## 2. Set env (Crossplatform)
* #### compile for 64 bit system    
    export AR=aarch64-linux-gnu-ar  
    export CC=aarch64-linux-gnu-gcc-12   
    export CXX=aarch64-linux-gnu-g++-12  
    export LINK=aarch64-linux-gnu-g++-12    
* #### compile for 32 bit system    
    export AR=arm-linux-gnueabihf-ar    
    export CC=arm-linux-gnueabihf-gcc-12    
    export CXX=arm-linux-gnueabihf-g++-12   
    export LINK=arm-linux-gnueabihf-g++-12   
## 3. Configure
npm run configure
## 4. Build Binary
npm run build
## 5. Build Lib
npm run esbuild