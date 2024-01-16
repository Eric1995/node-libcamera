const { H264Encoder } = require('../build/Release/camera.node');

const encoder = new H264Encoder(
  {
    width: 1920,
    height: 1080,
    bitrate: 20 * 1000 * 1000,
    level: 13,
    pixel_format: 842093913,
    bytesperline: 1920,
    invokeCallback: false,
  },
  (errorResponse, okResponse, progressData) => {
    if (progressData?.nalu === 5) {
      // console.log(`${count} --- ${progressData?.nalu} --- size: ${progressData?.data?.byteLength}`);
    }
    if (progressData) count++;
    if (progressData?.data) {
      // fs.appendFileSync('./test1.h264', Buffer.from(progressData?.data));
    }
  },
);
