import Stream from "./Stream";
import type { RawCamera } from "./types";

class Camera {
    private camera: RawCamera;
    constructor(_camera: RawCamera) {
        this.camera = _camera;
    }

    public getInfo() {
        return this.camera.getInfo();
    }

    public config(option: Parameters<RawCamera['config']>[0]) {
        return this.camera.config(option);
    }

    public createStreams(option: Parameters<RawCamera['createStreams']>[0]) {
        const streams =  this.camera.createStreams(option);
        return streams.map(s => new Stream(s));
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