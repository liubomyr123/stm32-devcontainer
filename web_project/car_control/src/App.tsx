import './App.css';

function App() {
  function send(cmd: string) {
    fetch('/cmd?v=' + cmd);
  }

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
      <h1>RC Car 11111</h1>
      <div>
        <button onClick={() => send('F:80')}>Forward</button>
      </div>
      <div>
        <button onClick={() => send('S')}>Stop</button>
      </div>
      <div>
        <button onClick={() => send('B:80')}>Backward</button>
      </div>
      <img
        id="stream"
        style={{
          display: 'none',
          width: '100%',
        }}
      />
      <button onClick={startStream}>Start</button>
      <button onClick={stopStream}>Stop</button>
    </div>
  );
}

export default App;
