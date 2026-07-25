import { useEffect, useRef } from 'react';
import nipplejs from 'nipplejs';
import { carStore } from '../class/CarStore.ts';
import moveSvg from '../assets/move.svg';
import camSvg from '../assets/cam.svg';
import type { JoystickType } from '../types';

function MyJoystick(props: { type: JoystickType }) {
  const joystickZoneRef = useRef<HTMLDivElement>(null);
  const isCameraJoystick = props.type == 'cam_control';

  useEffect(() => {
    if (!joystickZoneRef.current) return;

    const manager = nipplejs.create({
      zone: joystickZoneRef.current,
      mode: 'static',
      position: { left: '50%', top: '50%' },
      lockX: isCameraJoystick ? true : false,
      // restJoystick: false,
      color: isCameraJoystick
        ? {
            front: `url("${camSvg}") center/60% no-repeat, linear-gradient(135deg, #34d399, #10b981)`,
            back: 'rgba(16, 185, 129, 0.15)',
          }
        : {
            front: `url("${moveSvg}") center/60% no-repeat, linear-gradient(135deg, #38bdf8, #0ea5e9)`,
            back: 'rgba(99, 102, 241, 0.12)',
          },
    });

    manager.on('move', (evt) => {
      const { vector } = evt.data;

      const x = Math.round(vector.y * 100);
      const y = Math.round(vector.x * 100);

      let forward = 0;
      let backward = 0;
      let right = 0;
      let left = 0;

      if (x > 10) {
        forward = x;
      } else if (x < -10) {
        backward = Math.abs(x);
      }

      if (y > 10) {
        right = y;
      } else if (y < -10) {
        left = Math.abs(y);
      }

      // console.log(
      //   `F:${forward} B:${backward} R:${right} L:${left} x:${x} y:${y}`
      // );
      if (props.type === 'car_control') {
        carStore.updateCarControls({ forward, backward, right, left });
      } else if (props.type === 'cam_control') {
        carStore.updateCamControls({ panX: x, panY: y });
      }
    });

    manager.on('end', () => {
      // console.log(`S`);
      if (props.type === 'car_control') {
        carStore.updateCarControls({
          forward: 0,
          backward: 0,
          right: 0,
          left: 0,
        });
      } else if (props.type === 'cam_control') {
        carStore.updateCamControls({ panX: 0, panY: 0 });
      }
    });

    return () => manager.destroy();
  }, []);

  return (
    <div
      id={props.type}
      ref={joystickZoneRef}
      style={{
        width: '40vw',
        height: '40vw',
        maxWidth: '200px',
        maxHeight: '200px',
        position: 'relative',
        overflow: 'hidden',
        touchAction: 'none',
      }}
    />
  );
}

export default MyJoystick;
