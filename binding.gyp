{
    "targets": [
        {
            "target_name": "liblibcamera",
            'variables' : {
                # use cross platform compile or not
                'FLAG': '<!(echo $FLAG)',
            },
            "sources": [
                "cpp/index.cpp", 
                "cpp/utils/dma_heaps.cpp", 
                "cpp/image/dng.cpp", 
                "cpp/image/jpeg.cpp", 
                "cpp/image/qoi.cpp", 
                "cpp/image/jpeg_turbo.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "/usr/local/include/libcamera",
                "/usr/include/libcamera",
                "/opt/libjpeg-turbo/include",
                "/usr/include/aarch64-linux-gnu",
                "./include"
            ],
            "dependencies": [
                "<!@(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [],
            "cflags": [
                "-std=c++23",
                "-fpermissive",
                "-fexceptions",
                "-O1"
            ],  
            "cflags_cc": [
                "-std=c++23",
                "-fpermissive",
                "-fexceptions",
                "-O1"
            ],  
            "ldflags": [
                "-nolibc",
                "-static-libstdc++",
                "-static-libgcc"
            ],
            'defines': ["NAPI_CPP_EXCEPTIONS", "PI", "QOI_IMPLEMENTATION"],
            "conditions": [
                ['FLAG=="CROSS"', {
                    "libraries": [
                    "${PWD}/lib64/libcamera.so.0.3.2",
                    "${PWD}/lib64/libcamera-base.so.0.3.2",
                    "${PWD}/lib64/libexif.so.12.3.4",
                    "${PWD}/lib64/libjpeg.so.62.3.0",
                    "${PWD}/lib64/libtiff.so.6.0.0",
                    "${PWD}/lib64/libturbojpeg.so.0.4.0"
                    ],
                }],
                ['FLAG!="CROSS"', {
                    "libraries": [
                    "-lcamera",
                    "-lcamera-base",
                    "-lexif",
                    "-ljpeg"
                    ]
                }]
            ]
        }
    ]
}
