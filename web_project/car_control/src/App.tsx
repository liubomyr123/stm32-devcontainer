import { useEffect, useRef } from 'react';
import nipplejs from 'nipplejs';

function App() {
  const wsRef = useRef<WebSocket | null>(null);

  function connect() {
    wsRef.current = new WebSocket(`ws://${window.location.hostname}/ws`);

    wsRef.current.onopen = () => {
      console.log('WebSocket connected');
    };

    wsRef.current.onclose = () => {
      console.log('WebSocket disconnected');
      // спробуємо перепідключитись через 1 секунду
      setTimeout(connect, 1000);
    };

    wsRef.current.onerror = (err) => {
      console.error('WebSocket error', err);
    };
  }

  useEffect(() => {
    connect();
    return () => wsRef.current?.close();
  }, []);

  function send(cmd: string) {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(cmd);
    }
  }
  const joystickZoneRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!joystickZoneRef.current) return;

    const manager = nipplejs.create({
      zone: joystickZoneRef.current,
      mode: 'static',
      position: { left: '50%', top: '50%' },
      color: {
        front: 'linear-gradient(135deg, #34d399, #10b981)',
        back: 'rgba(16, 185, 129, 0.15)',
      },
    });

    manager.on('move', (evt) => {
      const { vector } = evt.data;

      const forwardSpeed = Math.round(vector.y * 100);
      const turnSpeed = Math.round(vector.x * 100);

      let forward = 0;
      let backward = 0;
      let right = 0;
      let left = 0;

      if (forwardSpeed > 10) {
        forward = forwardSpeed;
      } else if (forwardSpeed < -10) {
        backward = Math.abs(forwardSpeed);
      }

      if (turnSpeed > 10) {
        right = turnSpeed;
      } else if (turnSpeed < -10) {
        left = Math.abs(turnSpeed);
      }

      console.log(`F:${forward} B:${backward} R:${right} L:${left}`);
      send(`F:${forward} B:${backward} R:${right} L:${left}`);
    });

    manager.on('end', () => {
      console.log(`S`);
      send('S');
    });

    return () => manager.destroy();
  }, []);

  function startStream() {
    const img = document.getElementById('stream') as HTMLImageElement;
    img.style.display = 'block';
    img.src = '/stream?' + Date.now();
  }

  function stopStream() {
    fetch('/stream/stop');
    const img = document.getElementById('stream') as HTMLImageElement;
    img.style.display = 'none';
    img.src = '';
  }

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
      <h1>RC Car</h1>
      <img
        id="stream"
        style={{ display: 'none', width: '100%' }}
      />
      <button onClick={startStream}>Start</button>
      <button onClick={stopStream}>Stop</button>
      <div
        ref={joystickZoneRef}
        style={{
          width: '200px',
          height: '200px',
          margin: '20px auto',
          position: 'relative',
        }}
      />
    </div>
  );
}

export default App;
