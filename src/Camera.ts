import Image from './Image';
import Stream from './Stream';
import type { PixelFormat, RawCamera, RawCameraImage, StreamRole } from './types';

class Camera {
  private camera: RawCamera;
  constructor(_camera: RawCamera) {
    console.log('create camera');
    this.camera = _camera;
  }

  public getInfo() {
    return this.camera.getInfo();
  }

  public config(option: Parameters<RawCamera['config']>[0]) {
    return this.camera.config(option);
  }

  public createStreams(
    option: {
      width: number;
      height: number;
      role?: StreamRole;
      pixel_format?: PixelFormat;
      vflip?: boolean;
      hflip?: boolean;
      rotation?: number;
      bufferCount?: number;
      onImageData?: (err: unknown, ok: boolean, image: Image) => void;
    }[],
  ) {
    // overwrite onImageData
    const _option = option.map((o) => {
      // const _originalCall = o.onImageData;
      const _o = Object.assign({}, o) as Omit<typeof o, 'onImageData'> & {
        onImageData?: Parameters<RawCamera['createStreams']>[0][number]['onImageData'];
      };
      if (!o.onImageData) return _o;
      _o.onImageData = (err, ok, image) => {
        const _image = new Image(image);
        o.onImageData?.(err, ok, _image);
      };
      return _o;
    });
    const streams = this.camera.createStreams(_option);

    return streams.map((s) => {
      const st = new Stream(s);

      // st.config({
      //   onImageData: option[s.streamIndex].onImageData,
      // });
      return st;
    });
  }

  public start() {
    this.camera.start();
  }

  public stop() {
    this.camera.stop();
  }

  public release() {
    this.camera.release();
  }

  public sendRequest() {
    return this.camera.sendRequest();
  }

  public getAvailableControls() {
    return this.camera.getAvailableControls();
  }

  public setControls(controlls: Parameters<RawCamera['setControls']>[0]) {
    return this.camera.setControls(controlls);
  }

  public removeControl(controlls: Parameters<RawCamera['removeControl']>[0]) {
    return this.camera.removeControl(controlls);
  }

  public resetControls(controlls?: Parameters<RawCamera['resetControls']>[0]) {
    return this.camera.resetControls(controlls);
  }

  public get sensorModes() {
    return this.camera.sensorModes;
  }
}

export default Camera;
