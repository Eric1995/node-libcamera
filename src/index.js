const { CameraManager, H264Encoder } = require('../build/Release/camera.node');
const fs = require('fs');

let duration = 8000;
let framerate = 30;
const cm = new CameraManager(11);
const camera = cm.cameras[0];
camera.config({
  autoQueueRequest: true,
  maxFrameRate: framerate,
  onFrameData: (err, ok, imageList) => {
    // console.log('camera on frame data', imageList);
    // camera.queueRequest();
  },
});
let encoder;
let encoder2;
// console.log(camera.id);
console.log(camera.getInfo());
if (fs.existsSync('./test1.h264')) {
  fs.rmSync('./test1.h264');
}
if (fs.existsSync('./test2.h264')) {
  fs.rmSync('./test2.h264');
}
// if (fs.existsSync("./test.yuv")) {
//     fs.rmSync("./test.yuv");
// }
const configRes = camera.createStreams([
  {
    width: 1920,
    height: 1080,
    role: 'VideoRecording',
    pixel_format: 'YUV420',
    // pixel_format: 'SGBRG10_CSI2P',
  },
  // {
  //   width: 960,
  //   height: 720,
  //   // framerate: 30,
  //   role: 'Viewfinder',
  //   pixel_format: 'YUV420',
  //   // pixel_format: "SGBRG10_CSI2P",
  // },
  // {
  //   width: 320,
  //   height: 240,
  //   // framerate: 30,
  //   role: 'Raw',
  //   pixel_format: 'YUV420',
  //   // pixel_format: "SGBRG10_CSI2P",
  // },
]);

let stream0_count = 0;
let stream1_count = 0;
let stream2_count = 0;
let count = 0;
let count2 = 0;

configRes[0]?.configStream({
  auto_queue_request: true,
  // 0: return dma buffer fd and data;
  // 1: only return dma buffer fd;
  data_output_type: 1,
  onImageData: (err, ok, image) => {
    if (stream0_count === 0) {
      setTimeout(() => {
        camera.stop();
        // console.log(`try to stop encoder at: ${Date.now() / 1000}`);
        encoder2?.stop();
        encoder?.stop();
        // console.log(`encoder stopped at: ${Date.now() / 1000}`);
        // console.log(`stream0: ${stream0_count}, stream1: ${stream1_count} encoder0: ${count}, encoder2: ${count2}`);
      }, duration);
    }
    // console.log(`write yuv ${stream0_count}`);
    // console.dir(image.data);
    encoder?.feed(image.fd, image.frameSize);
    if (image.data) {
      // encoder?.feed({ size: image.frameSize, data: image.data });
      // fs.writeFile(`/dev/shm/yuv/${stream0_count}.yuv`, Buffer.from(image.data), () => {});
    }
    // encoder?.feed(image.fd, image.frameSize);
    // if(stream0_count % 60 === 0) {
    //     console.dir(stream0_count);
    //     image.toDNG(`raw${stream0_count}.dng`);
    // }
    // const filehandle = fs.openSync(`yuv/${stream0_count}.yuv`, 'r');
    // console.dir(filehandle);
    // encoder?.feed(filehandle, 640 * 480 * 1.5);
    // setTimeout(() => {
    //   fs.closeSync(filehandle);
    // }, 200);

    stream0_count++;
  },
});
configRes[1]?.configStream({
  auto_queue_request: false,
  data_output_type: 1,
  onImageData: (err, ok, image) => {
    stream1_count++;
    encoder2?.feed(image.fd, image.frameSize);
    // console.dir(image.frameSize);
  },
});

// if (configRes[2]) {
//   configRes[2].configStream({
//     auto_queue_request: false,
//     data_output_type: 1,
//     onImageData: (err, ok, image) => {
//       stream2_count++;
//       // encoder2.feed(image.fd, image.frameSize);
//       // console.dir(image.frameSize);
//     },
//   });
// }

// console.dir({...configRes[0]});
console.dir(camera.getAvailableControls());
camera.start();
// camera.queueRequest();
console.log(
  `stream 0: width: ${configRes[0]?.width}, height: ${configRes[0]?.height}, stride: ${configRes[0]?.stride}, pixel format: ${configRes[0]?.pixelFormtFourcc}`,
);
console.log(
  `stream 1: width: ${configRes[1]?.width}, height: ${configRes[1]?.height}, stride: ${configRes[1]?.stride}, pixel format: ${configRes[1]?.pixelFormtFourcc}`,
);
// console.log(`stream 0 pixel format: ${configRes[0]?.pixelFormat}, stream1 pixel format: ${configRes[1]?.pixelFormat}`);
if (configRes[0]) {
  encoder = new H264Encoder(
    {
      width: configRes[0].width,
      height: configRes[0].height,
      bitrate: 20 * 1000 * 1000,
      level: 13,
      pixel_format: configRes[0].pixelFormtFourcc,
      bytesperline: configRes[0].stride,
      invokeCallback: false,
      framerate: framerate,
      file: '/dev/shm/store.h264',
    },
    (errorResponse, okResponse, progressData) => {
      // console.log(`nalu: ${progressData?.nalu}`);
      if (progressData?.nalu === 5) {
        // console.log(`${count} --- ${progressData?.nalu} --- size: ${progressData?.data?.byteLength}`);
      }
      if (progressData) count++;
      if (progressData?.data) {
        // fs.appendFileSync('./test1.h264', Buffer.from(progressData?.data));
      }
    },
  );
}

if (configRes[1]) {
  encoder2 = new H264Encoder(
    {
      width: configRes[1].width,
      height: configRes[1].height,
      bitrate: 2.4 * 1000 * 1000,
      level: 13,
      pixel_format: configRes[1].pixelFormtFourcc,
      bytesperline: configRes[1].stride,
      framerate: framerate,
      // invokeCallback: false,
    },
    (errorResponse, okResponse, progressData) => {
      if(progressData?.nalu === 5) {
        console.log(progressData.data.byteLength);
      }
      if (progressData) count2++;
      if (progressData?.data) {
        // fs.appendFileSync('./test2.h264', Buffer.from(progressData.data));
      }
    },
  );
}

// set level to 4.2
// encoder2.setController({
//     code: 3221771804,
//     id: 10029671,
//     value: 13,

// });
// encoder.setController({
//     code: 3221771804,
//     id: 10029671,
//     value: 13,

// });
// // set profile to hight

encoder?.setController({
  code: 3221771804,
  id: 10029675,
  value: 4,
});
encoder2?.setController({
  code: 3221771804,
  id: 10029675,
  value: 4,
});

// encoder.setController({
//     //VIDIOC_S_CTRL
//     code: 3221771804,
//     //V4L2_CID_MPEG_VIDEO_BITRATE
//     id: 10029519,
//     // bitrate
//     value: 2 * 1000 * 1000,
// });

//set intra
encoder?.setController({
  //VIDIOC_S_CTRL
  code: 3221771804,
  id: 10029670,
  value: 30,
});
encoder2?.setController({
  //VIDIOC_S_CTRL
  code: 3221771804,
  id: 10029670,
  value: 30,
});

setTimeout(() => {
  // camera.config({
  //   // maxFrameRate: 30,
  //   crop: {
  //     roi_x: 0,
  //     roi_y: 0,
  //     roi_width: 0.5,
  //     roi_height: 0.5,
  //   },
  // });
  // encoder2.setController({
  //   //VIDIOC_S_CTRL
  //   code: 3221771804,
  //   //V4L2_CID_MPEG_VIDEO_BITRATE
  //   id: 10029519,
  //   // bitrate
  //   value: 4 * 1000 * 1000,
  // });
}, duration / 2);
setTimeout(() => {
  // console.log('waiting.......');

  console.log(`stream0: ${stream0_count}, stream1: ${stream1_count}, stream2: ${stream2_count} encoder0: ${count}, encoder2: ${count2}`);
}, duration + 500);
