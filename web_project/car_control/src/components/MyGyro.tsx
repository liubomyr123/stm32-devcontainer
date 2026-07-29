import { useEffect, useRef, useState } from 'react';
import { useGyroControls } from '../hooks/useGyroControls';

function drawAI(canvas: HTMLCanvasElement, pitch: number, roll: number) {
  const clampedPitch = Math.max(-90, Math.min(90, pitch));
  const clampedRoll = Math.max(-180, Math.min(180, roll));

  const ctx = canvas.getContext('2d')!;
  const W = canvas.width,
    R = W / 2;

  ctx.clearRect(0, 0, W, W);
  ctx.save();
  ctx.beginPath();
  ctx.arc(R, R, R - 1, 0, Math.PI * 2);
  ctx.clip();

  ctx.save();
  ctx.translate(R, R);
  ctx.rotate((clampedRoll * Math.PI) / 180);

  const pitchPx = clampedPitch * 3;

  const skyGrad = ctx.createLinearGradient(0, -R * 2, 0, pitchPx);
  skyGrad.addColorStop(0, '#5ba3d9');
  skyGrad.addColorStop(1, '#a8d4f5');
  ctx.fillStyle = skyGrad;
  ctx.fillRect(-R * 2, -R * 2, R * 4, R * 2 + pitchPx);

  const groundGrad = ctx.createLinearGradient(0, pitchPx, 0, R * 2);
  groundGrad.addColorStop(0, '#8b9b2e');
  groundGrad.addColorStop(1, '#5a6b1a');
  ctx.fillStyle = groundGrad;
  ctx.fillRect(-R * 2, pitchPx, R * 4, R * 4);

  ctx.strokeStyle = '#4caf50';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(-R * 2, pitchPx);
  ctx.lineTo(R * 2, pitchPx);
  ctx.stroke();

  ctx.lineWidth = 1.5;
  ctx.font = '11px sans-serif';
  ctx.textAlign = 'right';
  for (let p = -30; p <= 30; p += 5) {
    if (p === 0) continue;
    const y = pitchPx - p * 3;
    const isMajor = p % 10 === 0;
    const len = isMajor ? 50 : 30;
    ctx.strokeStyle = 'rgba(255,255,255,0.85)';
    ctx.lineWidth = isMajor ? 1.5 : 1;
    ctx.beginPath();
    ctx.moveTo(-len, y);
    ctx.lineTo(len, y);
    ctx.stroke();
    if (isMajor) {
      ctx.strokeStyle = 'rgba(0,0,0,0.8)';
      ctx.lineWidth = 3;
      ctx.lineJoin = 'round';
      ctx.strokeText(String(p), -len - 4, y + 4);
      ctx.fillStyle = '#ffffff';
      ctx.fillText(String(p), -len - 4, y + 4);
    }
  }
  ctx.restore();

  // Pitch out of range indicator
  if (clampedPitch > 30) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3;
    ctx.font = 'bold 14px sans-serif';
    ctx.textAlign = 'center';
    ctx.strokeText(`▲ ${pitch.toFixed(0)}°`, 0, -R + 70);
    ctx.fillText(`▲ ${pitch.toFixed(0)}°`, 0, -R + 70);
    ctx.restore();
  } else if (clampedPitch < -30) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3;
    ctx.font = 'bold 14px sans-serif';
    ctx.textAlign = 'center';
    ctx.strokeText(`▼ ${pitch.toFixed(0)}°`, 0, R - 70);
    ctx.fillText(`▼ ${pitch.toFixed(0)}°`, 0, R - 70);
    ctx.restore();
  }

  ctx.save();
  ctx.translate(R, R);
  ctx.strokeStyle = 'rgba(255,255,255,0.8)';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(0, 0, R - 30, -Math.PI * 0.85, -Math.PI * 0.15);
  ctx.stroke();

  const tickAngles = [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60];
  const labelAngles = [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60];

  tickAngles.forEach((a) => {
    const rad = ((a - 90) * Math.PI) / 180;
    const isMajor = a % 30 === 0;
    const len = isMajor ? 12 : 6;
    const r1 = R - 30;
    const r2 = r1 + len;
    ctx.strokeStyle = 'rgba(255,255,255,0.8)';
    ctx.lineWidth = isMajor ? 2 : 1;
    ctx.beginPath();
    ctx.moveTo(Math.cos(rad) * r1, Math.sin(rad) * r1);
    ctx.lineTo(Math.cos(rad) * r2, Math.sin(rad) * r2);
    ctx.stroke();
  });

  ctx.font = '10px sans-serif';
  ctx.fillStyle = '#ffffff';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';

  labelAngles.forEach((a) => {
    const rad = ((a - 90) * Math.PI) / 180;
    const rLabel = R - 12; // ← було R - 38
    const x = Math.cos(rad) * rLabel;
    const y = Math.sin(rad) * rLabel;
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(rad + Math.PI / 2);
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3;
    ctx.lineJoin = 'round';
    ctx.strokeText(String(Math.abs(a)), 0, 0);
    ctx.fillText(String(Math.abs(a)), 0, 0);
    ctx.restore();
  });

  ctx.rotate((clampedRoll * Math.PI) / 180);
  ctx.fillStyle = '#fd0000';
  ctx.strokeStyle = '#fd0000';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(0, -(R - 30));
  ctx.lineTo(-8, -(R - 38));
  ctx.lineTo(8, -(R - 38));
  ctx.closePath();
  ctx.stroke();
  ctx.restore();

  ctx.save();
  ctx.translate(R, R);
  ctx.strokeStyle = '#e53935';
  ctx.lineWidth = 3;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(-65, 0);
  ctx.lineTo(-22, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(22, 0);
  ctx.lineTo(65, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(-22, 0);
  ctx.lineTo(-10, 8);
  ctx.lineTo(0, 0);
  ctx.lineTo(10, 8);
  ctx.lineTo(22, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.arc(0, 0, 4, 0, Math.PI * 2);
  ctx.fillStyle = '#e53935';
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(-R + 10, 0);
  ctx.lineTo(-R + 32, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(R - 10, 0);
  ctx.lineTo(R - 32, 0);
  ctx.stroke();
  ctx.restore();

  ctx.restore();

  // Roll value at bottom
  if (Math.abs(roll) > 60) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3;
    ctx.font = 'bold 12px sans-serif';
    ctx.textAlign = 'center';
    const rollArrow = roll > 0 ? '►' : roll < 0 ? '◄' : '';
    ctx.strokeText(`${rollArrow} ${roll.toFixed(0)}°`, 0, R - 15);
    ctx.fillText(`${rollArrow} ${roll.toFixed(0)}°`, 0, R - 15);
    ctx.restore();
  }
}

function AttitudeIndicator({ pitch, roll }: { pitch: number; roll: number }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (canvasRef.current) {
      drawAI(canvasRef.current, pitch, roll);
    }
  }, [pitch, roll]);

  return (
    <canvas
      ref={canvasRef}
      width={270}
      height={270}
      style={{ borderRadius: '50%', border: '2px solid #555' }}
    />
  );
}

function MyGyro() {
  const gyro = useGyroControls();
  const [manualPitch, setManualPitch] = useState(0);
  const [manualRoll, setManualRoll] = useState(0);

  return (
    <div
      style={{
        display: 'flex',
        gap: 32,
        alignItems: 'flex-start',
        flexWrap: 'wrap',
      }}
    >
      {/* Live */}
      <div
        style={{
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          gap: 8,
        }}
      >
        <AttitudeIndicator
          pitch={gyro.pitch}
          roll={gyro.roll}
        />
        {/* <div style={{ fontSize: 13, color: '#888', display: 'flex', gap: 16 }}>
          <span>P: {gyro.pitch.toFixed(1)}°</span>
          <span>R: {gyro.roll.toFixed(1)}°</span>
        </div> */}
      </div>

      {/* Manual */}
      <div
        style={{
          display: 'none',
          flexDirection: 'column',
          alignItems: 'center',
          gap: 8,
        }}
      >
        <AttitudeIndicator
          pitch={manualPitch}
          roll={manualRoll}
        />
        <div style={{ fontSize: 13, color: '#888', display: 'flex', gap: 16 }}>
          <span>P: {manualPitch.toFixed(0)}°</span>
          <span>R: {manualRoll.toFixed(0)}°</span>
        </div>
        <div
          style={{
            display: 'flex',
            flexDirection: 'column',
            gap: 6,
            width: 200,
          }}
        >
          <label style={{ fontSize: 12, color: '#888' }}>pitch</label>
          <input
            type="range"
            min={-180}
            max={180}
            value={manualPitch}
            onChange={(e) => setManualPitch(Number(e.target.value))}
          />
          <label style={{ fontSize: 12, color: '#888' }}>roll</label>
          <input
            type="range"
            min={-180}
            max={180}
            value={manualRoll}
            onChange={(e) => setManualRoll(Number(e.target.value))}
          />
        </div>
      </div>
    </div>
  );
}

export default MyGyro;
