const { CameraManager, H264Encoder } = require('../build/Release/camera.node');
const fs = require('fs');

let duration = 8000;
let framerate = 30;
const cm = new CameraManager(42);
const camera = cm.cameras[0];

let store_count = 0;
let still_count = 0;

function createStoreStream() {
  camera.config({
    autoQueueRequest: true,
    maxFrameRate: framerate,
  });
  const configRes = camera.createStreams([
    {
      width: 1920,
      height: 1080,
      role: 'VideoRecording',
      pixel_format: 'YUV420',
    },
  ]);
  configRes[0]?.configStream({
    auto_queue_request: true,
    data_output_type: 1,
    onImageData: (err, ok, image) => {
      // console.dir(image);
      if (store_count === 0) {
        
      }
      store_count++;
    },
  });
}

function captureImage() {
  camera.config({
    autoQueueRequest: true,
    maxFrameRate: 9,
  });
  const configRes = camera.createStreams([
    {
      width: 2592,
      height: 1944,
      role: 'VideoRecording',
      pixel_format: 'YUV420',
    },
  ]);

  configRes[0]?.configStream({
    auto_queue_request: true,
    data_output_type: 1,
    onImageData: (err, ok, image) => {
      // console.dir(image);
      still_count++;
    },
  });
  camera.start();
  setTimeout(() => {
    camera.stop();
    camera.release();
    console.log(`store count: ${store_count}, still count: ${still_count}`);
    console.log('progress exit');
    // setTimeout(() => {
    //   console.log('exit');
    // }, 2000);
  }, 4000);
}

createStoreStream();
camera.start();
setTimeout(() => {
  console.log('stop camera');
  console.log(`store count: ${store_count}, still count: ${still_count}`);
  camera.stop();
  setTimeout(() => {
    captureImage();
  }, 1000);
}, 4000);
