export enum StreamRole {
  Raw = 0,
  StillCapture = 1,
  VideoRecording = 2,
  Viewfinder = 3,
}

export enum PixelFormat {
  R8 = 'R8',
  R10 = 'R10',
  R12 = 'R12',
  R16 = 'R16',
  RGB565 = 'RGB565',
  RGB565_BE = 'RGB565_BE',
  RGB888 = 'RGB888',
  BGR888 = 'BGR888',
  XRGB8888 = 'XRGB8888',
  XBGR8888 = 'XBGR8888',
  RGBX8888 = 'RGBX8888',
  BGRX8888 = 'BGRX8888',
  ARGB8888 = 'ARGB8888',
  ABGR8888 = 'ABGR8888',
  RGBA8888 = 'RGBA8888',
  BGRA8888 = 'BGRA8888',
  RGB161616 = 'RGB161616',
  BGR161616 = 'BGR161616',
  YUYV = 'YUYV',
  YVYU = 'YVYU',
  UYVY = 'UYVY',
  VYUY = 'VYUY',
  AVUY8888 = 'AVUY8888',
  XVUY8888 = 'XVUY8888',
  NV12 = 'NV12',
  NV21 = 'NV21',
  NV16 = 'NV16',
  NV61 = 'NV61',
  NV24 = 'NV24',
  NV42 = 'NV42',
  YUV420 = 'YUV420',
  YVU420 = 'YVU420',
  YUV422 = 'YUV422',
  YVU422 = 'YVU422',
  YUV444 = 'YUV444',
  YVU444 = 'YVU444',
  MJPEG = 'MJPEG',
  SRGGB8 = 'SRGGB8',
  SGRBG8 = 'SGRBG8',
  SGBRG8 = 'SGBRG8',
  SBGGR8 = 'SBGGR8',
  SRGGB10 = 'SRGGB10',
  SGRBG10 = 'SGRBG10',
  SGBRG10 = 'SGBRG10',
  SBGGR10 = 'SBGGR10',
  SRGGB12 = 'SRGGB12',
  SGRBG12 = 'SGRBG12',
  SGBRG12 = 'SGBRG12',
  SBGGR12 = 'SBGGR12',
  SRGGB14 = 'SRGGB14',
  SGRBG14 = 'SGRBG14',
  SGBRG14 = 'SGBRG14',
  SBGGR14 = 'SBGGR14',
  SRGGB16 = 'SRGGB16',
  SGRBG16 = 'SGRBG16',
  SGBRG16 = 'SGBRG16',
  SBGGR16 = 'SBGGR16',
  R10_CSI2P = 'R10_CSI2P',
  R12_CSI2P = 'R12_CSI2P',
  SRGGB10_CSI2P = 'SRGGB10_CSI2P',
  SGRBG10_CSI2P = 'SGRBG10_CSI2P',
  SGBRG10_CSI2P = 'SGBRG10_CSI2P',
  SBGGR10_CSI2P = 'SBGGR10_CSI2P',
  SRGGB12_CSI2P = 'SRGGB12_CSI2P',
  SGRBG12_CSI2P = 'SGRBG12_CSI2P',
  SGBRG12_CSI2P = 'SGBRG12_CSI2P',
  SBGGR12_CSI2P = 'SBGGR12_CSI2P',
  SRGGB14_CSI2P = 'SRGGB14_CSI2P',
  SGRBG14_CSI2P = 'SGRBG14_CSI2P',
  SGBRG14_CSI2P = 'SGBRG14_CSI2P',
  SBGGR14_CSI2P = 'SBGGR14_CSI2P',
  SRGGB10_IPU3 = 'SRGGB10_IPU3',
  SGRBG10_IPU3 = 'SGRBG10_IPU3',
  SGBRG10_IPU3 = 'SGBRG10_IPU3',
  SBGGR10_IPU3 = 'SBGGR10_IPU3',
  RGGB_PISP_COMP1 = 'RGGB_PISP_COMP1',
  GRBG_PISP_COMP1 = 'GRBG_PISP_COMP1',
  GBRG_PISP_COMP1 = 'GBRG_PISP_COMP1',
  BGGR_PISP_COMP1 = 'BGGR_PISP_COMP1',
  MONO_PISP_COMP1 = 'MONO_PISP_COMP1',
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
    hflip?: boolean;
    vflip?: boolean;
    rotation?: number;
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
