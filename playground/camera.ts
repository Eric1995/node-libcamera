import { PixelFormat, type RawCameraManagerConstruction } from '../src/types';
import { createRequire } from 'node:module';
const require = createRequire(import.meta.url);
const { CameraManager } = require('../build/Release/camera.node') as {
    CameraManager: RawCameraManagerConstruction;
};

const cm = new CameraManager();
const camera = cm.cameras[0];
camera.stop();
camera.config({
    autoQueueRequest: true,
    maxFrameRate: 30,
});
const configRes = camera.createStreams([
    {
        width: 1920,
        height: 1080,
        role: 2,
        pixel_format: PixelFormat.YUV420,
        onImageData: (err, ok, image) => {
            
        },
    },
]);
console.dir(configRes);
camera.start();
setTimeout(() => {
    camera.stop();
}, 4000);