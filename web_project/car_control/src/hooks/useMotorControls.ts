import { useState, useEffect } from 'react';
import { carStore } from '../class/CarStore';

export function useMotorControls() {
  const [motors, setMotors] = useState(carStore.getMotorControls());

  useEffect(() => {
    const unsubscribe = carStore.subscribe(() => {
      setMotors(carStore.getMotorControls());
    });
    return unsubscribe;
  }, []);

  return motors;
}
