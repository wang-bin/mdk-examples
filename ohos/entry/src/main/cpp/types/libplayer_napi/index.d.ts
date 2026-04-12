export const setMedia: (url: string) => void;
export const play: () => void;
export const pause: () => void;
export const stop: () => void;
export const seek: (ms: number) => void;
export const setPlaybackRate: (rate: number) => void;
export const setVolume: (volume: number) => void;
export const getPosition: () => number;
export const getDuration: () => number;
export const isPlaying: () => boolean;
export const setProperty: (key: string, value: string) => void;

export const enum ColorSpace {
  Unknown = 0,
  BT709 = 1,
  BT2100_PQ = 2,
}
export const setColorSpace: (colorSpace: ColorSpace) => void;
