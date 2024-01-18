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
      onImageData: (err, ok, image) => {
        // console.log(`callback called at: ${Date.now()}, size: ${image.frameSize}`);
        store_count++;
      },
    },
  ]);
  // configRes.forEach((stream) => {
  //   console.dir(stream);
  //   for (const k in stream) {
  //     console.dir(`${k}: ${stream[k]}`);
  //   }
  // });
  // configRes[0]?.configStream({
  //   auto_queue_request: true,
  //   data_output_type: 1,
  //   onImageData: (err, ok, image) => {
  //     // console.dir(image);
  //     if (store_count === 0) {
  //     }
  //     store_count++;
  //   },
  // });
}

function captureImage() {
  camera.config({
    autoQueueRequest: true,
    maxFrameRate: 4,
  });
  const configRes = camera.createStreams([
    {
      width: 1920,
      height: 1080,
      role: 'Raw',
      pixel_format: 'SGBRG10_CSI2P',
      // pixel_format: 'YUV420',
      onImageData: () => {
        // console.log(`callback called at: ${Date.now()}`);
        still_count++;
      },
    },
  ]);

  configRes[0]?.configStream({
    data_output_type: 1,
    onImageData: (err, ok, image) => {
      if (still_count === 0) {
        console.log(`save image to dng`);
        image.toDNG(`./${still_count}.dng`, () => {
          console.log('dng saved');
        });
      }
      if (still_count === 1) {
        console.log(`save image to jpeg`);
        image.toJPEG(`./${still_count}.jpeg`, () => {
          console.log('jpeg saved');
        });
      }
      still_count++;
    },
  });
  // camera.start();
  // setTimeout(() => {
  //   camera.stop();
  //   camera.release();
  //   console.log(`store count: ${store_count}, still count: ${still_count}`);
  //   console.log('progress exit');
  //   // setTimeout(() => {
  //   //   console.log('exit');
  //   // }, 2000);
  // }, 4000);
}

createStoreStream();
camera.start();
setTimeout(() => {
  console.log(`stop camera, store count: ${store_count}, still count: ${still_count}`);
  camera.stop();
  captureImage();
  camera.start();
  setTimeout(() => {
    camera.stop();
    console.log(`stop camera, store count: ${store_count}, still count: ${still_count}`);
  }, 4000);
  // setTimeout(() => {
  //   captureImage();
  // }, 1000);
}, 4000);
