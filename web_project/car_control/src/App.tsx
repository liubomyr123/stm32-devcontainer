import { useState } from 'react';
import MyJoystick from './components/MyJoystick';
import { useCarConnection } from './hooks/useCarConnection';
import MyCamera from './components/MyCamera';
import MyGyro from './components/MyGyro';
import Settings from './components/Settings.tsx';
import './App.css';

function App() {
  useCarConnection();
  const [settingsOpen, setSettingsOpen] = useState(false);

  return (
    <div className="app">
      <div className="app__header">
        <h2 className="app__title">Car Controls</h2>
        <button
          className="app__settings-btn"
          onClick={() => setSettingsOpen(true)}
          aria-label="Settings"
        >
          ⚙
        </button>
      </div>

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

      {settingsOpen && (
        <>
          <div
            className="app__overlay"
            onClick={() => setSettingsOpen(false)}
          />
          <div className="app__settings-panel">
            <div className="app__settings-panel-header">
              <span>Settings</span>
              <button
                className="app__settings-close"
                onClick={() => setSettingsOpen(false)}
                aria-label="Close"
              >
                ✕
              </button>
            </div>
            <Settings />
          </div>
        </>
      )}
    </div>
  );
}

export default App;
