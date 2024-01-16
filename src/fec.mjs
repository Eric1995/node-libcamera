import fs from 'fs';
const wasmBuffer = fs.readFileSync('../zfec/build/zfec.wasm');
WebAssembly.instantiate(wasmBuffer, {
  env: {
    memoryBase: 0,
    tableBase: 0,
    memory: new WebAssembly.Memory({ initial: 64 }),
  },
}).then((wasmModule) => {
  // Exported function live under instance.exports
  const exports = wasmModule.instance.exports;
  console.dir(exports);
});
