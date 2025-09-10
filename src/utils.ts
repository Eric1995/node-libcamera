export function fourccToNumber(fourcc: string): number {
  if (fourcc.length !== 4) {
    throw new Error('FourCC code must be 4 characters long.');
  }

  const a = fourcc.charCodeAt(0);
  const b = fourcc.charCodeAt(1);
  const c = fourcc.charCodeAt(2);
  const d = fourcc.charCodeAt(3);

  // V4L2 默认使用 little-endian 字节序
  const value = a | (b << 8) | (c << 16) | (d << 24);

  return value;
}

export function numberToFourcc(value: number): string {
  const a = value & 0xff;
  const b = (value >> 8) & 0xff;
  const c = (value >> 16) & 0xff;
  const d = (value >> 24) & 0xff;

  return String.fromCharCode(a, b, c, d);
}
