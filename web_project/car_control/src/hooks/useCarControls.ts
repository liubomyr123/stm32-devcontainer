import { useState, useEffect } from 'react';
import { carStore } from '../class/CarStore';

export function useCarControls() {
  const [controls, setControls] = useState(carStore.getCarControls());

  useEffect(() => {
    const unsubscribe = carStore.subscribe(() => {
      setControls(carStore.getCarControls());
    });
    return unsubscribe;
  }, []);

  return controls;
}
