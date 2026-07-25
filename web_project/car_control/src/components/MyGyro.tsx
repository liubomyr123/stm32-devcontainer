import { useGyroControls } from '../hooks/useGyroControls';

function MyGyro() {
  const gyro = useGyroControls();

  return (
    <div
      style={{
        width: '150px',
        minHeight: '150px',
        height: '100%',
        border: '1px solid #444',
        borderRadius: '10px',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'center',
        alignItems: 'center',
        gap: '10px',
      }}
    >
      <div>X: {gyro.x}</div>
      <div>Y: {gyro.y}</div>
      <div>Z: {gyro.z}</div>
    </div>
  );
}

export default MyGyro;
