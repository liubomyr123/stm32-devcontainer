import type {
  CamControls,
  CarControls,
  GyroControls,
  MotorControls,
} from '../types';

class CarStore {
  private static instance: CarStore;

  private carControls: CarControls = {
    forward: 0,
    backward: 0,
    right: 0,
    left: 0,
  };

  private camControls: CamControls = {
    panX: 0,
    panY: 0,
  };

  private gyroControls: GyroControls = {
    pitch: 0,
    roll: 0,
  };

  private motorControls: MotorControls = {
    fl: 0,
    fr: 0,
    rl: 0,
    rr: 0,
  };

  private listeners: Set<() => void> = new Set();

  private constructor() {}

  static getInstance(): CarStore {
    if (!CarStore.instance) {
      CarStore.instance = new CarStore();
    }
    return CarStore.instance;
  }

  updateCarControls(carControls: Partial<CarControls>) {
    this.carControls = { ...this.carControls, ...carControls };
    this.notify();
  }

  updateCamControls(camControls: Partial<CamControls>) {
    this.camControls = { ...this.camControls, ...camControls };
    this.notify();
  }

  updateGyroControls(gyroControls: Partial<GyroControls>) {
    this.gyroControls = { ...this.gyroControls, ...gyroControls };
    this.notify();
  }

  updateMotorControls(motorControls: Partial<MotorControls>) {
    this.motorControls = { ...this.motorControls, ...motorControls };
    this.notify();
  }

  getMotorControls(): MotorControls {
    return this.motorControls;
  }

  getCarControls(): CarControls {
    return this.carControls;
  }

  getCamControls(): CamControls {
    return this.camControls;
  }

  getGyroControls(): GyroControls {
    return this.gyroControls;
  }

  subscribe(listener: () => void): () => void {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  private notify() {
    this.listeners.forEach((listener) => listener());
  }
}

export const carStore = CarStore.getInstance();
