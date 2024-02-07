import Stream from "./Stream";
import type { RawCamera } from "./types";

class Camera {
    camera: RawCamera;
    constructor(_camera: RawCamera) {
        this.camera = _camera;
    }

    getInfo() {
        return this.camera.getInfo();
    }

    config(option: Parameters<RawCamera['config']>[0]) {
        return this.camera.config(option);
    }

    createStreams(option: Parameters<RawCamera['createStreams']>[0]) {
        const streams =  this.camera.createStreams(option);
        return streams.map(s => new Stream(s));
    }

    start() {
        this.camera.start();
    }

    stop() {
        this.camera.stop();
    }

    release() {
        this.camera.release();
    }
}

export default Camera;