import { useEffect, useRef } from 'react';
import { carStore } from '../class/CarStore';
import { useWebSocket } from './useWebSocket';

export function useCarConnection() {
  const { send, onMessage } = useWebSocket();
  const lastSentRef = useRef('');

  useEffect(() => {
    onMessage((data) => {
      try {
        const telemetry = JSON.parse(data);
        console.log(telemetry);
        if (
          typeof telemetry.pitch === 'number' &&
          typeof telemetry.roll === 'number'
        ) {
          carStore.updateGyroControls({
            pitch: telemetry.pitch,
            roll: telemetry.roll,
          });
        }
      } catch {
        console.warn('Invalid telemetry:', data);
      }
    });
  }, []);

  useEffect(() => {
    const interval = setInterval(() => {
      const controls = carStore.getCarControls();
      const cam = carStore.getCamControls();

      const packet = JSON.stringify({
        f: controls.forward,
        b: controls.backward,
        r: controls.right,
        l: controls.left,
        px: cam.panX,
        py: cam.panY,
      });

      if (packet !== lastSentRef.current) {
        console.log(packet);
        lastSentRef.current = packet;
        send(packet);
      }
    }, 50);

    return () => clearInterval(interval);
  }, []);
}
