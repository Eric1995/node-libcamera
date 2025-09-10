import type { RawCameraImage } from './types';
import { numberToFourcc } from './utils';

class Image {
  private image: RawCameraImage;
  constructor(_image: RawCameraImage) {
    this.image = _image;
  }

  public get fd() {
    return this.image.fd;
  }

  public getData() {
    return this.image.getData();
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

  public get pixelFormatFourcc() {
    return this.image.pixelFormatFourcc;
  }

  public get pixelFormatFourccString() {
    return numberToFourcc(this.image.pixelFormatFourcc);
  }

  public save(option: Parameters<RawCameraImage['save']>[0], callback?: Parameters<RawCameraImage['save']>[1]) {
    return this.image.save(option, callback);
  }
}

export default Image;
