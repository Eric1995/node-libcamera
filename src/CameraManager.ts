import { createRequire } from 'module';
import Camera from './Camera';
import type { RawCameraManager, RawCameraManagerConstruction } from './types';
const require = createRequire(import.meta?.url ?? __filename);

const addonPath = process.env.NODE_ENV === 'production' ? '../build/Release/libcamera.node' : '../dist/build/Release/libcamera.node';
const { CameraManager: RawCameraManager } = require(addonPath) as {
  CameraManager: RawCameraManagerConstruction;
};

class CameraManager {
  private cm: RawCameraManager;
  public cameras: Camera[] = [];
  constructor() {
    this.cameras = [];
    this.cm = new RawCameraManager();
    this.cm.cameras.forEach((c) => {
      this.cameras.push(new Camera(c));
    });
  }
}

export default CameraManager;
