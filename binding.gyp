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
            
            'defines': ["NAPI_CPP_EXCEPTIONS", "PI", "QOI_IMPLEMENTATION"],
            "conditions": [
                ['FLAG=="CROSS"', {
                    "libraries": [
                        "${PWD}/lib64/*",
                    ],
                    "ldflags": [
                        "-nolibc",
                        "-static-libstdc++",
                        "-static-libgcc"
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
