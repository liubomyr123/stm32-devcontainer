export type JoystickType = 'car_control' | 'cam_control';

export interface CarControls {
  forward: number;
  backward: number;
  right: number;
  left: number;
}

export interface GyroControls {
  pitch: number;
  roll: number;
}

export interface CamControls {
  panX: number;
  panY: number;
}
