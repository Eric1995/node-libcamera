export enum StreamRole {
  Raw = 0,
  StillCapture = 1,
  VideoRecording = 2,
  Viewfinder = 3,
}

export enum PixelFormat {
  RGB565 = 'RGB565',
  RGB888 = 'RGB888',
  YUV420 = 'YUV420',
  SGBRG10_CSI2P = 'SGBRG10_CSI2P',
  SRGGB10_CSI2P = 'SRGGB10_CSI2P',
}

export enum AeMeteringModeEnum {
  MeteringCentreWeighted = 0,
  MeteringSpot = 1,
  MeteringMatrix = 2,
  MeteringCustom = 3,
}

export enum AeExposureModeEnum {
  ExposureNormal = 0,
  ExposureShort = 1,
  ExposureLong = 2,
  ExposureCustom = 3,
}

export enum AwbModeEnum {
  AwbAuto = 0,
  AwbIncandescent = 1,
  AwbTungsten = 2,
  AwbFluorescent = 3,
  AwbIndoor = 4,
  AwbDaylight = 5,
  AwbCloudy = 6,
  AwbCustom = 7,
}

export enum AfModeEnum {
  AfModeManual = 0,
  AfModeAuto = 1,
  AfModeContinuous = 2,
}

export interface ControlList {
  ExposureTime?: number;
  AnalogueGain?: number;
  AeMeteringMode?: AeMeteringModeEnum;
  AeExposureMode?: AeExposureModeEnum;
  ExposureValue?: number;
  AwbMode?: AwbModeEnum;
  Brightness?: number;
  Contrast?: number;
  Saturation?: number;
  Sharpness?: number;
  AfRange?: number;
  AfSpeed?: number;
  LensPosition?: number;
  AfMode?: AfModeEnum;
  AfTrigger?: number;
}

export interface RawCameraImage {
  fd: number;
  frameSize: number;
  getData: () => ArrayBuffer;
  colorSpace: string;
  stride: number;
  pixelFormat: PixelFormat;
  pixelFormatFourcc: number;
  save: (
    option: {
      file: string;
      /**
       * @description
       * 1: save as DNG;
       * 2: save as JPEG;
       * 3: save as YUV;
       * 4: save as QOI;
       */
      type: 1 | 2 | 3 | 4;
      /**
       * @description
       * number, 1 - 100
       * @default 
       * 90
       */
      quality?: number;
    },
    callback?: () => void,
  ) => void;
}

export interface RawCamera {
  id: string;
  config: (option: {
    autoQueueRequest?: boolean;
    maxFrameRate?: number;
    crop?: {
      roi_x: number;
      roi_y: number;
      roi_width: number;
      roi_height: number;
    };
  }) => number;
  getInfo: () => {
    id: string;
    width: number;
    height: number;
    model: string;
    bit: number;
    colorFilterArrangement: string;
    modes: Record<
      string,
      {
        size: string;
        pix: string;
        crop: string;
        framerate: number;
      }
    >;
  };
  createStreams: (
    options: {
      width: number;
      height: number;
      role?: StreamRole;
      pixel_format?: PixelFormat;
      onImageData?: (err: unknown, ok: boolean, image: RawCameraImage) => void;
    }[],
  ) => RawCameraStream[];
  start: () => number;
  stop: () => boolean;
  sendRequest: () => number;
  release: () => boolean;
  getAvailableControls: () => {
    [k in keyof ControlList]: string;
  };
  setControl: (controlls: ControlList) => undefined;
  sensorModes: {
    size: string;
    pix: string;
    framerate: number;
  }[];
}

export interface RawCameraManager {
  cameras: RawCamera[];
}

export interface RawCameraManagerConstruction {
  new (): RawCameraManager;
}

export interface RawCameraStream {
  stride: number;
  colorSpace: string;
  pixelFormat: PixelFormat;
  pixelFormtFourcc: number;
  frameSize: number;
  width: number;
  height: number;
  streamIndex: number;
  configStream: (option: { onImageData?: (err: unknown, ok: boolean, image: RawCameraImage) => void }) => number;
}
