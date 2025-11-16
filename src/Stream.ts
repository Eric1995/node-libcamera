import Image from './Image';
import type { RawCameraStream } from './types';
import { numberToFourcc } from './utils';

class Stream {
  private stream: RawCameraStream;
  constructor(_stream: RawCameraStream) {
    this.stream = _stream;
  }

  public get stride() {
    return this.stream.stride;
  }

  public get colorSpace() {
    return this.stream.colorSpace;
  }

  public get pixelFormat() {
    return this.stream.pixelFormat;
  }

  public get pixelFormtFourcc() {
    return numberToFourcc(this.stream.pixelFormtFourcc);
  }

  public get pixelFormtFourccNumber() {
    return this.stream.pixelFormtFourcc;
  }

  public get frameSize() {
    return this.stream.frameSize;
  }

  public get width() {
    return this.stream.width;
  }

  public get height() {
    return this.stream.height;
  }

  public get index() {
    return this.stream.streamIndex;
  }

  public config(
    option: Omit<Parameters<RawCameraStream['configStream']>[0], 'onImageData' | 'data_output_type'> & {
      /**
       * @description
       * data output format
       * 0: fd and buffer;
       * 1: fd only;
       * @default 1
       */
      // dataOutputType?: 0 | 1;
      onImageData?: (err: unknown, ok: boolean, image: Image) => void;
    },
  ) {
    if (!option || typeof option !== 'object') return;
    const newOption = { ...option, onImageData: undefined } as Parameters<RawCameraStream['configStream']>[0];
    if (option.onImageData && typeof option.onImageData === 'function') {
      newOption.onImageData = (err: string, ok, image) => {
        const _image = new Image(image);
        option.onImageData?.(err, ok, _image);
      };
    }
    return this.stream.configStream(newOption);
  }
}
export default Stream;
