{
    "targets": [
        {
            "target_name": "camera",
            "sources": ["camera/cpp/index.cpp", "camera/cpp/dma_heaps.cpp", "camera/cpp/image/dng.cpp", "camera/cpp/image/jpeg.cpp"],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "/usr/local/include/libcamera",
                "/usr/include/libcamera",
                "/home/eric/Dev/picam/include",
                "/usr/aarch64-linux-gnu/include",
            ],
            "dependencies": [
                "<!@(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [
                # "-lcamera",
                # "-lcamera-base",
                "/home/eric/Dev/node-libcamera/lib64/libcamera.so",
                "/home/eric/Dev/node-libcamera/lib64/libcamera-base.so",
                "/home/eric/Dev/node-libcamera/lib64/libtiff.so.6.0.0",
                "/home/eric/Dev/node-libcamera/lib64/libexif.so.12.3.4",
                # "/home/eric/Dev/node-libcamera/lib64/libexif.a",
                "/home/eric/Dev/node-libcamera/lib64/libjpeg.so.62.3.0",
                # "/home/eric/Dev/node-libcamera/lib64/libexif.so.12.3.4",
                # "/home/eric/Dev/picam/lib/libdng.a"
                # "-lpthread"
                # "/home/eric/Dev/picam/lib32/libcamera.so",
                # "/home/eric/Dev/picam/lib32/libcamera-base.so",
                # "/home/eric/Dev/picam/lib32/libtiff.so.6.0.0",
            ],
            "cflags": [
                "-std=c++20",
                "-fpermissive",
                "-fexceptions",
                "-g",
                "-O0",
                # "-static-libgcc",
                # "-static-libstdc++",
            ],  
            "cflags_cc": [
                "-std=c++20",
                "-fpermissive",
                "-fexceptions",
                "-g",
                "-O0",
                # "-static-libgcc",
                # "-static-libstdc++"
            ],  
            'defines': ["NAPI_CPP_EXCEPTIONS", "PI"],
        },
    ]
}
