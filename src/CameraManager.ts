

import Camera from './Camera';
import type { RawCameraManager, RawCameraManagerConstruction } from './types';

// eslint-disable-next-line @typescript-eslint/no-var-requires
const { CameraManager: _CameraManager } = require('../build/Release/camera.node') as {
  CameraManager: RawCameraManagerConstruction;
};

class CameraManager {
  private cm: RawCameraManager;
  public cameras: Camera[] = [];
  constructor() {
    this.cameras = [];
    this.cm = new _CameraManager();
    this.cm.cameras.forEach((c) => {
      this.cameras.push(new Camera(c));
    });
  }
}

export default CameraManager;
