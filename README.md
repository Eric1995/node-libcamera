# node-libcamera
nodejs binding for libcamera

## 1. Install applications
sudo apt install gcc-12-aarch64-linux-gnu g++-12-aarch64-linux-gnu sshpass
sudo apt install libcamera-dev libtiff-dev libexif-dev
## 2. Set env
export AR=aarch64-linux-gnu-ar  
export CC=aarch64-linux-gnu-gcc-12   
export CXX=aarch64-linux-gnu-g++-12  
export LINK=aarch64-linux-gnu-g++-12      
## 3. Configure
npm run configure
## 4. Build Binary
npm run build
## 5. Build Lib
npm run esbuild