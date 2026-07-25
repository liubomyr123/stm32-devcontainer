import { useState } from 'react';

function MyCamera() {
  const [isStreaming, setIsStreaming] = useState(false);

  function startStream() {
    const img = document.getElementById('stream') as HTMLImageElement;
    img.style.display = 'block';
    img.src = '/stream?' + Date.now();
    setIsStreaming(true);
  }

  function stopStream() {
    fetch('/stream/stop');
    const img = document.getElementById('stream') as HTMLImageElement;
    img.style.display = 'none';
    img.src = '';
    setIsStreaming(false);
  }

  return (
    <div>
      <div
        style={{
          height: '200px',
          background: '#111',
          border: '1px solid #444',
          borderRadius: '8px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          overflow: 'hidden',
          position: 'relative',
        }}
      >
        <img
          id="stream"
          style={{
            display: 'none',
            width: '100%',
            height: '100%',
            objectFit: 'cover',
          }}
        />
        {!isStreaming && <span style={{ color: '#666' }}>Camera stopped</span>}
      </div>
      <button onClick={startStream}>Start</button>
      <button onClick={stopStream}>Stop</button>
    </div>
  );
}

export default MyCamera;
