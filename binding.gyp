{
    "targets": [
        {
            "target_name": "camera",
            "sources": ["camera/cpp/index.cpp", "cpp/utils/dma_heaps.cpp", "cpp/image/dng.cpp", "cpp/image/jpeg.cpp"],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "/usr/local/include/libcamera",
                "/usr/include/libcamera",
                "/usr/include/aarch64-linux-gnu",
                "./include"
            ],
            "dependencies": [
                "<!@(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [
                "/home/eric/Dev/node-libcamera/lib64/libcamera.so",
                "/home/eric/Dev/node-libcamera/lib64/libcamera-base.so",
                "/home/eric/Dev/node-libcamera/lib64/libtiff.so.6.0.0",
                "/home/eric/Dev/node-libcamera/lib64/libexif.so.12.3.4",
                "/home/eric/Dev/node-libcamera/lib64/libjpeg.so.62.3.0",
            ],
            "cflags": [
                "-std=c++20",
                "-fpermissive",
                "-fexceptions",
            ],  
            "cflags_cc": [
                "-std=c++20",
                "-fpermissive",
                "-fexceptions",
            ],  
            'defines': ["NAPI_CPP_EXCEPTIONS", "PI"],
        }
    ]
}
