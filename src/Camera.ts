import Image from './Image';
import Stream from './Stream';
import type { PixelFormat, RawCamera, StreamRole } from './types';

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
      onImageData?: (err: unknown, ok: boolean, image: Image) => void;
    }[],
  ) {
    const streams = this.camera.createStreams(option);
    // immediately overwrite onImageData
    return streams.map((s) => {
      const st = new Stream(s);
      st.config({
        onImageData: option[s.streamIndex].onImageData,
      });
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
}

export default Camera;
