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

export enum AeStateEnum {
  AeStateIdle = 0,
  AeStateSearching = 1,
  AeStateConverged = 2,
}

export enum AeMeteringModeEnum {
  MeteringCentreWeighted = 0,
  MeteringSpot = 1,
  MeteringMatrix = 2,
  MeteringCustom = 3,
}

export enum AeConstraintModeEnum {
  ConstraintNormal = 0,
  ConstraintHighlight = 1,
  ConstraintShadows = 2,
  ConstraintCustom = 3,
}

export enum AeExposureModeEnum {
  ExposureNormal = 0,
  ExposureShort = 1,
  ExposureLong = 2,
  ExposureCustom = 3,
}

export enum ExposureTimeModeEnum {
  ExposureTimeModeAuto = 0,
  ExposureTimeModeManual = 1,
}

export enum AnalogueGainModeEnum {
  AnalogueGainModeAuto = 0,
  AnalogueGainModeManual = 1,
}

export enum AeFlickerModeEnum {
  FlickerOff = 0,
  FlickerManual = 1,
  FlickerAuto = 2,
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

export enum AfSpeedEnum {
  AfSpeedNormal = 0,
  AfSpeedFast = 1,
}

export enum AfMeteringEnum {
  AfMeteringAuto = 0,
  AfMeteringWindows = 1,
}

export enum AfTriggerEnum {
  AfTriggerStart = 0,
  AfTriggerCancel = 1,
}

export enum AfPauseEnum {
  AfPauseImmediate = 0,
  AfPauseDeferred = 1,
  AfPauseResume = 2,
}

export enum AfStateEnum {
  AfStateIdle = 0,
  AfStateScanning = 1,
  AfStateFocused = 2,
  AfStateFailed = 3,
}

export enum AfPauseStateEnum {
  AfPauseStateRunning = 0,
  AfPauseStatePausing = 1,
  AfPauseStatePaused = 2,
}

export enum HdrModeEnum {
  HdrModeOff = 0,
  HdrModeMultiExposureUnmerged = 1,
  HdrModeMultiExposure = 2,
  HdrModeSingleExposure = 3,
  HdrModeNight = 4,
}

export enum HdrChannelEnum {
  HdrChannelNone = 0,
  HdrChannelShort = 1,
  HdrChannelMedium = 2,
  HdrChannelLong = 3,
}

// export interface ControlListIn {
//   AeEnable?: boolean;
//   AeMeteringMode?: AeMeteringModeEnum;
//   AeConstraintMode?: AeConstraintModeEnum;
//   AeExposureMode?: AeExposureModeEnum;
//   ExposureValue?: number;
//   ExposureTime?: number;
//   ExposureTimeMode?: ExposureTimeModeEnum;
//   AnalogueGain?: number;
//   AnalogueGainMode?: AnalogueGainModeEnum;
//   AeFlickerMode?: AeFlickerModeEnum;
//   AeFlickerPeriod?: number;
//   Brightness?: number;
//   Contrast?: number;
//   AwbEnable?: boolean;
//   AwbMode?: AwbModeEnum;
//   ColourGains?: [number, number];
//   Saturation?: number;
//   Sharpness?: number;
//   ColourCorrectionMatrix?: [number, number, number, number, number, number, number, number, number];
//   ScalerCrop?: [number, number, number, number] | { x: number; y: number; width: number; height: number };
//   DigitalGain?: number;
//   FrameDurationLimits?: [number, number];
//   AfMode?: AfModeEnum;
//   AfRange?: number;
//   AfSpeed?: AfSpeedEnum;
//   AfMetering?: AfMeteringEnum;
//   AfWindows?: [number, number, number, number][];
//   AfTrigger?: AfTriggerEnum;
//   AfPause?: AfPauseEnum;
//   LensPosition?: number;
//   HdrMode?: HdrModeEnum;
//   Gamma?: number;
//   DebugMetadataEnable?: boolean;
// }

export interface ControlList {
  AeEnable?: boolean;
  AeState?: AeStateEnum;
  AeMeteringMode?: AeMeteringModeEnum;
  AeConstraintMode?: AeConstraintModeEnum;
  AeExposureMode?: AeExposureModeEnum;
  ExposureValue?: number;
  ExposureTime?: number;
  ExposureTimeMode?: ExposureTimeModeEnum;
  AnalogueGain?: number;
  AnalogueGainMode?: AnalogueGainModeEnum;
  AeFlickerMode?: AeFlickerModeEnum;
  AeFlickerPeriod?: number;
  AeFlickerDetected?: number;
  Brightness?: number;
  Contrast?: number;
  Lux?: number;
  AwbEnable?: boolean;
  AwbMode?: AwbModeEnum;
  AwbLocked?: boolean;
  ColourGains?: [number, number];
  ColourTemperature?: number;
  Saturation?: number;
  SensorBlackLevels?: [number, number, number, number];
  Sharpness?: number;
  FocusFoM?: number;
  ColourCorrectionMatrix?: [number, number, number, number, number, number, number, number, number];
  ScalerCrop?: { x: number; y: number; width: number; height: number };
  DigitalGain?: number;
  FrameDuration?: number;
  FrameDurationLimits?: [number, number];
  SensorTemperature?: number;
  SensorTimestamp?: number;
  AfMode?: AfModeEnum;
  AfRange?: number;
  AfSpeed?: AfSpeedEnum;
  AfMetering?: AfMeteringEnum;
  AfWindows?: { x: number; y: number; width: number; height: number }[];
  AfTrigger?: AfTriggerEnum;
  AfPause?: AfPauseEnum;
  LensPosition?: number;
  AfState?: AfStateEnum;
  AfPauseState?: AfPauseStateEnum;
  HdrMode?: HdrModeEnum;
  HdrChannel?: HdrChannelEnum;
  Gamma?: number;
  DebugMetadataEnable?: boolean;
  FrameWallClock?: number;
}

export type ControlListIn = Omit<
  ControlList,
  | 'AeState'
  | 'AeFlickerDetected'
  | 'Lux'
  | 'AwbLocked'
  | 'ColourTemperature'
  | 'SensorBlackLevels'
  | 'FocusFoM'
  | 'FrameDuration'
  | 'SensorTemperature'
  | 'SensorTimestamp'
  | 'AfState'
  | 'AfPauseState'
  | 'HdrChannel'
  | 'FrameWallClock'
>;
export type ControlListOut = Omit<ControlList, 'AeEnable' | 'AfTrigger' | 'AfPause'>;

export interface RawCameraImage {
  fd: number;
  frameSize: number;
  getData: () => ArrayBuffer;
  getMetadata: () => ControlListOut;
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
    sensorMode?: { width: number; height: number; bitDepth: number; packed?: boolean };
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
  setControls: (controlls: ControlListIn) => undefined;
  removeControl: (controlls: keyof ControlListIn) => undefined;
  resetControls: (controlls?: (keyof ControlListIn)[]) => undefined;
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
