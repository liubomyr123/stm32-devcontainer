export type JoystickType = 'car_control' | 'cam_control';

export interface CarControls {
  forward: number;
  backward: number;
  right: number;
  left: number;
}

export interface GyroControls {
  x: number;
  y: number;
  z: number;
}

export interface CamControls {
  panX: number;
  panY: number;
}
