import { useState } from 'react';
import '../styles/MyCamera.css';

function MyCamera() {
  const [isStreaming, setIsStreaming] = useState(false);

  function startStream() {
    const img = document.getElementById('stream') as HTMLImageElement;
    img.src = '/stream?' + Date.now();
    setIsStreaming(true);
  }

  function stopStream() {
    fetch('/stream/stop');
    const img = document.getElementById('stream') as HTMLImageElement;
    img.src = '';
    setIsStreaming(false);
  }

  return (
    <div>
      <div className="camera-frame">
        <img
          id="stream"
          className={`camera-frame__stream ${isStreaming ? 'camera-frame__stream--active' : ''}`}
        />
        {!isStreaming && (
          <span className="camera-frame__placeholder">Camera stopped</span>
        )}
      </div>
      <button onClick={startStream}>Start</button>
      <button onClick={stopStream}>Stop</button>
    </div>
  );
}

export default MyCamera;
