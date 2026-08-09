import { useEffect, useState } from 'react';
import '../styles/Settings.css';

type TestStatus = 'idle' | 'testing' | 'success' | 'failed';
type SaveStatus = 'idle' | 'saved' | 'failed';

function Settings() {
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');

  const [savedSsid, setSavedSsid] = useState('');
  const [savedPassword, setSavedPassword] = useState('');

  const [staMode, setStaMode] = useState(false);
  const [testStatus, setTestStatus] = useState<TestStatus>('idle');
  const [saveStatus, setSaveStatus] = useState<SaveStatus>('idle');
  const [showPassword, setShowPassword] = useState(false);

  const [staConnecting, setStaConnecting] = useState(false);
  const [displayIp, setDisplayIp] = useState<string | null>(null);

  const getCanSave = () => {
    return (
      ssid.trim() &&
      password.trim() &&
      (ssid !== savedSsid || password !== savedPassword)
    );
  };

  const handleSave = async () => {
    if (!getCanSave()) {
      return;
    }

    try {
      const res = await fetch('/wifi/credentials', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password }),
      });
      if (res.ok) {
        setSavedSsid(ssid);
        setSavedPassword(password);
        setSaveStatus('saved');
        setTimeout(() => setSaveStatus('idle'), 2000);
      } else {
        console.error('Save failed:', await res.text());
        setSaveStatus('failed');
        setTimeout(() => setSaveStatus('idle'), 2000);
      }
    } catch (err) {
      console.error('Save request failed:', err);
      setSaveStatus('failed');
      setTimeout(() => setSaveStatus('idle'), 2000);
    }
  };

  const handleTest = async () => {
    if (!ssid.trim() || !password.trim() || staMode) {
      return;
    }

    setTestStatus('testing');

    try {
      const res = await fetch('/wifi/test', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password }),
      });

      if (res.ok) {
        const data = await res.json();
        setTestStatus(data.connected ? 'success' : 'failed');
      } else {
        console.error('Test failed:', await res.text());
        setTestStatus('failed');
      }
    } catch (err) {
      console.error('Test request failed:', err);
      setTestStatus('failed');
    } finally {
      setTimeout(() => setTestStatus('idle'), 3000);
    }
  };

  const handleToggle = async () => {
    setStaConnecting(true);

    try {
      if (staMode) {
        const res = await fetch('/wifi/ap', { method: 'POST' });
        if (res.ok) {
          const data = await res.json();
          setStaMode(false);
          setDisplayIp(data.ip);
        } else {
          console.error('Failed to switch to AP:', await res.text());
        }
      } else {
        const res = await fetch('/wifi/sta', { method: 'POST' });
        if (res.ok) {
          const data = await res.json();
          setStaMode(data.connected);
          if (data.connected) {
            setDisplayIp(data.ip);
          } else {
            console.error('STA connection failed, stayed on AP');
          }
        } else {
          console.error('Failed to switch to STA:', await res.text());
        }
      }
    } catch (err) {
      console.error('Toggle request failed:', err);
    } finally {
      setStaConnecting(false);
    }
  };

  useEffect(() => {
    fetch('/wifi/credentials')
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then((data) => {
        if (data.has_saved) {
          setSsid(data.ssid);
          setPassword(data.password);
          setSavedSsid(data.ssid);
          setSavedPassword(data.password);
        }
      })
      .catch((err) => console.error('Failed to load credentials:', err));
  }, []);

  useEffect(() => {
    fetch('/wifi/status')
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then((data) => {
        setStaMode(data.sta_active);
        setDisplayIp(data.ip);
      })
      .catch((err) => console.error('Failed to load wifi status:', err));
  }, []);

  const getToggleLabel = () => {
    if (staConnecting) {
      return 'Switching...';
    }
    if (staMode) {
      return 'Control via shared network (STA)';
    }
    return 'Direct control (AP)';
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
          disabled={!getCanSave()}
        >
          Save
        </button>

        <button
          onClick={handleTest}
          className="wifi-settings__btn"
          disabled={!ssid || !password || testStatus === 'testing' || staMode}
        >
          {testStatus === 'testing' ? 'Testing...' : 'Test STA'}
        </button>
      </div>

      {saveStatus === 'saved' && (
        <div className="wifi-settings__message wifi-settings__message--success">
          Saved
        </div>
      )}
      {saveStatus === 'failed' && (
        <div className="wifi-settings__message wifi-settings__message--error">
          Failed to save
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
        <span>{getToggleLabel()}</span>
        <label className="wifi-settings__switch">
          <input
            type="checkbox"
            checked={staMode}
            onChange={handleToggle}
            disabled={staConnecting}
          />
          <span className="wifi-settings__slider" />
        </label>
      </div>

      {displayIp && (
        <div className="wifi-settings__ip-notice">
          Open:{' '}
          <a
            href={`http://${displayIp}`}
            target="_blank"
            rel="noopener noreferrer"
          >
            http://{displayIp}
          </a>
        </div>
      )}
    </div>
  );
}

export default Settings;
