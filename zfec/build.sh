#/bin/bash
rm -rf ./build
mkdir build
emcc \
-g \
-fdebug-compilation-dir='file:///home/eric/Dev/picam/zfec' \
--no-entry \
src/fec.c \
-DNDEBUG \
-s WASM=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s PURE_WASI \
-o build/zfec.wasm \
-s EXPORTED_RUNTIME_METHODS=ccall,cwrap \
-s EXPORTED_FUNCTIONS="['_malloc', '_free', '_fec_free','_fec_init', '_fec_new','_fec_encode','_fec_decode']"
# -------------------------------------------------------------------------
# emcc \
# src/add.cpp src/fec.c \
# -DNDEBUG \
# -O0 \
# -s WASM=1 \
# -s MINIFY_HTML=0 \
# -s TOTAL_MEMORY=10485760  \
# -o build/test.html \
# -s EXPORTED_RUNTIME_METHODS=ccall,cwrap \
# -s EXPORTED_FUNCTIONS="['_malloc', '_free', '_fec_free','_fec_init', '_fec_new','_fec_encode','_fec_decode']"

cp src/index.html build/index.html