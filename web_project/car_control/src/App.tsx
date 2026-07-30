import MyJoystick from './components/MyJoystick';
import { useCarConnection } from './hooks/useCarConnection';
import MyCamera from './components/MyCamera';
import MyGyro from './components/MyGyro';
import './App.css';

function App() {
  useCarConnection();

  return (
    <div className="app">
      <h2 className="app__title">Car Controls</h2>

      <div className="app__top-row">
        <div className="app__camera-col">
          <MyCamera />
        </div>
        <div className="app__gyro-col">
          <MyGyro />
        </div>
      </div>

      <div className="app__controls-row">
        <MyJoystick type="car_control" />
        <MyJoystick type="cam_control" />
      </div>
    </div>
  );
}

export default App;
