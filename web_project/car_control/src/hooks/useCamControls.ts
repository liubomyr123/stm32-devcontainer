import { useState, useEffect } from 'react';
import { carStore } from '../class/CarStore';

export function useCamControls() {
  const [controls, setControls] = useState(carStore.getCamControls());

  useEffect(() => {
    const unsubscribe = carStore.subscribe(() => {
      setControls(carStore.getCamControls());
    });
    return unsubscribe;
  }, []);

  return controls;
}
