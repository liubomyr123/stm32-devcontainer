import { useState } from 'react';
import '../styles/Settings.css';

type TestStatus = 'idle' | 'testing' | 'success' | 'failed';

function Settings() {
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [staMode, setStaMode] = useState(false);
  const [testStatus, setTestStatus] = useState<TestStatus>('idle');
  const [saveStatus, setSaveStatus] = useState<'idle' | 'saved'>('idle');
  const [showPassword, setShowPassword] = useState(false);

  const handleSave = () => {
    // TODO: POST /wifi/credentials
    setSaveStatus('saved');
    setTimeout(() => setSaveStatus('idle'), 2000);
  };

  const handleTest = () => {
    // TODO: POST /wifi/test
    setTestStatus('testing');
  };

  const handleToggle = () => {
    // TODO: POST /wifi/sta or /wifi/ap depending on new state
    setStaMode((prev) => !prev);
  };

  return (
    <div className="wifi-settings">
      <h3 className="wifi-settings__title">Shared Wi-Fi Network</h3>

      <div className="wifi-settings__field">
        <label htmlFor="wifi-ssid">SSID</label>
        <input
          id="wifi-ssid"
          name="wifi-ssid-field"
          type="text"
          autoComplete="off"
          value={ssid}
          onChange={(e) => setSsid(e.target.value)}
          placeholder="Network name"
        />
      </div>

      <div className="wifi-settings__field">
        <label htmlFor="wifi-password">Password</label>
        <div className="wifi-settings__password-wrap">
          <input
            id="wifi-password"
            name="wifi-password-field"
            type={showPassword ? 'text' : 'password'}
            autoComplete="new-password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Network password"
          />
          <button
            type="button"
            className="wifi-settings__password-toggle"
            onClick={() => setShowPassword((prev) => !prev)}
            aria-label={showPassword ? 'Hide password' : 'Show password'}
          >
            {showPassword ? '🙈' : '👁'}
          </button>
        </div>
      </div>

      <div className="wifi-settings__actions">
        <button
          onClick={handleSave}
          className="wifi-settings__btn"
        >
          Save
        </button>

        <button
          onClick={handleTest}
          className="wifi-settings__btn"
          disabled={!ssid || !password || testStatus === 'testing'}
        >
          {testStatus === 'testing' ? 'Testing...' : 'Test STA'}
        </button>
      </div>

      {saveStatus === 'saved' && (
        <div className="wifi-settings__message wifi-settings__message--success">
          Saved
        </div>
      )}

      {testStatus === 'success' && (
        <div className="wifi-settings__message wifi-settings__message--success">
          Connection successful
        </div>
      )}
      {testStatus === 'failed' && (
        <div className="wifi-settings__message wifi-settings__message--error">
          Failed to connect
        </div>
      )}

      <div className="wifi-settings__toggle-row">
        <span>
          {staMode ? 'Control via shared network (STA)' : 'Direct control (AP)'}
        </span>
        <label className="wifi-settings__switch">
          <input
            type="checkbox"
            checked={staMode}
            onChange={handleToggle}
          />
          <span className="wifi-settings__slider" />
        </label>
      </div>
    </div>
  );
}

export default Settings;
