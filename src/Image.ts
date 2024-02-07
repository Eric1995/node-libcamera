import type { RawCameraImage } from './types';

class Image {
  image: RawCameraImage;
  constructor(_image: RawCameraImage) {
    this.image = _image;
  }

  public get fd() {
    return this.image.fd;
  }

  public get data() {
    return this.image.data;
  }

  public get stride() {
    return this.image.stride;
  }

  public get colorSpace() {
    return this.image.colorSpace;
  }

  public get pixelFormat() {
    return this.image.pixelFormat;
  }

  public get frameSize() {
    return this.image.frameSize;
  }

  public save(option: Parameters<RawCameraImage['save']>[0], callback?: Parameters<RawCameraImage['save']>[1]) {
    return this.image.save(option, callback);
  }
}

export default Image;
