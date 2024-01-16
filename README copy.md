# 1. Install applications
sudo apt install gcc-12-aarch64-linux-gnu g++-12-aarch64-linux-gnu sshpass
# 2. Set env
export AR=aarch64-linux-gnu-ar 
export CC=aarch64-linux-gnu-gcc-12 
export CXX=aarch64-linux-gnu-g++-12 
export LINK=aarch64-linux-gnu-g++-12  
# 3. Configure
npm run configure
# 4. Build
npm run build