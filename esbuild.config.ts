import { execSync } from 'child_process';
import esbuild from 'esbuild';
import fs from 'fs-extra';

const outputPath = 'dist';

if (fs.existsSync(outputPath)) fs.rmSync(outputPath, { recursive: true });

esbuild.buildSync({
  entryPoints: ['src/index.ts'],
  outdir: `${outputPath}/es/`,
  format: 'esm',
  bundle: true,
  platform: 'node',
  tsconfig: './tsconfig.json',
  target: 'ESNext',
  external: ['../build/Release/camera.node'],
  banner: {
    js: `
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
    `,
  },
});

esbuild.buildSync({
  entryPoints: ['src/index.ts'],
  outdir: `${outputPath}/lib/`,
  format: 'cjs',
  bundle: true,
  platform: 'node',
  tsconfig: './tsconfig.json',
  target: 'ESNext',
  external: ['../build/Release/camera.node'],
});

try {
  execSync(`tsc --emitDeclarationOnly --declaration --project tsconfig.json --outDir ${outputPath}/types`);
} catch (error) {
  console.error(error.stdout.toString());
}

fs.copySync(`${outputPath}/types`, `${outputPath}/es`);
// fs.copySync(`${outputPath}/types/src`, `${outputPath}/lib`);
fs.rmSync(`${outputPath}/types`, { recursive: true });

if (fs.existsSync('build/Release/camera.node')) {
  fs.copySync(`build/Release/camera.node`, `${outputPath}/build/Release/camera.node`);
}
