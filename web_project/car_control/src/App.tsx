import MyJoystick from './components/MyJoystick';
import { useCarConnection } from './hooks/useCarConnection';
import MyCamera from './components/MyCamera';
import MyGyro from './components/MyGyro';

function App() {
  useCarConnection();

  return (
    <div
      style={{
        fontFamily: 'Arial',
        textAlign: 'center',
        background: '#1a1a1a',
        color: 'white',
        minHeight: '100vh',
      }}
    >
      <h1
        style={{
          padding: '20px',
        }}
      >
        Car Controls
      </h1>
      <div
        style={{
          display: 'flex',
          alignItems: 'stretch',
          gap: '10px',
          padding: '10px',
        }}
      >
        <div style={{ flex: 3 }}>
          <MyCamera />
        </div>
        <div style={{ flex: 1 }}>
          <MyGyro />
        </div>
      </div>
      <div
        style={{
          display: 'flex',
          justifyContent: 'space-around',
          alignItems: 'center',
          flex: 1,
          padding: '10px',
        }}
      >
        <MyJoystick type="car_control" />
        <MyJoystick type="cam_control" />
      </div>
    </div>
  );
}

export default App;
