import { RawCameraManagerConstruction } from './types';

// 这个文件只负责加载.node文件并导出
const addonPath = process.env.NODE_ENV === 'production' ? '../build/Release/libcamera.node' : '../dist/build/Release/libcamera.node';
const addon = require(addonPath) as {
  CameraManager: RawCameraManagerConstruction;
};
export default addon;
