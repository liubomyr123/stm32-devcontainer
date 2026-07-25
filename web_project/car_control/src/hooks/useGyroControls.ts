import { useState, useEffect } from 'react';
import { carStore } from '../class/CarStore';

export function useGyroControls() {
  const [gyro, setGyro] = useState(carStore.getGyroControls());

  useEffect(() => {
    const unsubscribe = carStore.subscribe(() => {
      setGyro(carStore.getGyroControls());
    });
    return unsubscribe;
  }, []);

  return gyro;
}
