{
    "variables" : {
        # use cross platform compile or not
        "FLAG": "<!(echo $FLAG)",
        "TARGET_ARCH": "<!(echo $TARGET_ARCH)",
    },
    "targets": [
        {
            "target_name": "liblibcamera",
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
                "./include"
            ],
            "dependencies": [
                "<!@(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [],  
            "cflags_cc": [
                "-std=c++23",
                "-fpermissive",
                "-fexceptions",
                "-O1"
            ],  
            
            "defines": ["NAPI_CPP_EXCEPTIONS", "PI", "QOI_IMPLEMENTATION"],
            "conditions": [
                ["FLAG=='CROSS'", {
                    "ldflags": [ "-nolibc", "-static-libstdc++", "-static-libgcc" ],
                    "conditions": [
                        ["TARGET_ARCH=='arm64' or TARGET_ARCH=='aarch64'", {
                            "include_dirs": ["/usr/include/aarch64-linux-gnu"],
                            "libraries": ["${PWD}/lib64/*"],
                            "cflags_cc": ["-target", "aarch64-linux-gnu"],  
                            "ldflags": ["-target", "aarch64-linux-gnu"]  
                        }],
                        ["TARGET_ARCH=='arm'", {
                            "include_dirs": ["/usr/include/arm-linux-gnueabihf"],
                            "libraries": ["${PWD}/lib32/*"],
                            "cflags_cc": ["-target", "arm-linux-gnueabi"],        
                            "ldflags": ["-target", "arm-linux-gnueabi"]                      
                        }],
                    ],
                }],
                ["FLAG!='CROSS'", {
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
