import { useEffect, useRef } from 'react';
import { carStore } from '../class/CarStore';
import { useWebSocket } from './useWebSocket';

export function useCarConnection() {
  const { send, onMessage } = useWebSocket();
  const lastSentRef = useRef('');

  useEffect(() => {
    onMessage((data) => {
      const telemetry = JSON.parse(data);
      carStore.updateGyroControls({
        x: telemetry.gx,
        y: telemetry.gy,
        z: telemetry.gz,
      });
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
