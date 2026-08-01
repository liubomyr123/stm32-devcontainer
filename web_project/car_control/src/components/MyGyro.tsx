import { useEffect, useRef } from 'react';
import { useGyroControls } from '../hooks/useGyroControls';
import { useIsMobile } from '../hooks/useIsMobile';
import { useMotorControls } from '../hooks/useMotorControls';
import '../styles/MyGyro.css';

const BASE_RADIUS = 135; // еталонний радіус, під який підібрані всі пропорції

function drawAI(canvas: HTMLCanvasElement, pitch: number, roll: number) {
  const clampedPitch = Math.max(-90, Math.min(90, pitch));
  const clampedRoll = Math.max(-180, Math.min(180, roll));
  const ctx = canvas.getContext('2d')!;
  const W = canvas.width,
    R = W / 2;
  const scale = R / BASE_RADIUS;

  ctx.clearRect(0, 0, W, W);
  ctx.save();
  ctx.beginPath();
  ctx.arc(R, R, R - 1, 0, Math.PI * 2);
  ctx.clip();
  ctx.save();
  ctx.translate(R, R);
  ctx.rotate((clampedRoll * Math.PI) / 180);
  const pitchPx = clampedPitch * 3 * scale;
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
  ctx.lineWidth = 2 * scale;
  ctx.beginPath();
  ctx.moveTo(-R * 2, pitchPx);
  ctx.lineTo(R * 2, pitchPx);
  ctx.stroke();

  ctx.font = `${11 * scale}px sans-serif`;
  ctx.textAlign = 'right';
  for (let p = -30; p <= 30; p += 5) {
    if (p === 0) continue;
    const y = pitchPx - p * 3 * scale;
    const isMajor = p % 10 === 0;
    const len = (isMajor ? 50 : 30) * scale;
    ctx.strokeStyle = 'rgba(255,255,255,0.85)';
    ctx.lineWidth = (isMajor ? 1.5 : 1) * scale;
    ctx.beginPath();
    ctx.moveTo(-len, y);
    ctx.lineTo(len, y);
    ctx.stroke();
    if (isMajor) {
      ctx.strokeStyle = 'rgba(0,0,0,0.8)';
      ctx.lineWidth = 3 * scale;
      ctx.lineJoin = 'round';
      ctx.strokeText(String(p), -len - 4 * scale, y + 4 * scale);
      ctx.fillStyle = '#ffffff';
      ctx.fillText(String(p), -len - 4 * scale, y + 4 * scale);
    }
  }
  ctx.restore();

  if (clampedPitch > 30) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3 * scale;
    ctx.font = `bold ${14 * scale}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.strokeText(`▲ ${pitch.toFixed(0)}°`, 0, -R + 70 * scale);
    ctx.fillText(`▲ ${pitch.toFixed(0)}°`, 0, -R + 70 * scale);
    ctx.restore();
  } else if (clampedPitch < -30) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3 * scale;
    ctx.font = `bold ${14 * scale}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.strokeText(`▼ ${pitch.toFixed(0)}°`, 0, R - 70 * scale);
    ctx.fillText(`▼ ${pitch.toFixed(0)}°`, 0, R - 70 * scale);
    ctx.restore();
  }

  ctx.save();
  ctx.translate(R, R);
  ctx.strokeStyle = 'rgba(255,255,255,0.8)';
  ctx.lineWidth = 1.5 * scale;
  ctx.beginPath();
  ctx.arc(0, 0, R - 30 * scale, -Math.PI * 0.85, -Math.PI * 0.15);
  ctx.stroke();

  const tickAngles = [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60];
  tickAngles.forEach((a) => {
    const rad = ((a - 90) * Math.PI) / 180;
    const isMajor = a % 30 === 0;
    const len = (isMajor ? 12 : 6) * scale;
    const r1 = R - 30 * scale;
    const r2 = r1 + len;
    ctx.strokeStyle = 'rgba(255,255,255,0.8)';
    ctx.lineWidth = (isMajor ? 2 : 1) * scale;
    ctx.beginPath();
    ctx.moveTo(Math.cos(rad) * r1, Math.sin(rad) * r1);
    ctx.lineTo(Math.cos(rad) * r2, Math.sin(rad) * r2);
    ctx.stroke();
  });

  ctx.font = `${10 * scale}px sans-serif`;
  ctx.fillStyle = '#ffffff';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  tickAngles.forEach((a) => {
    const rad = ((a - 90) * Math.PI) / 180;
    const rLabel = R - 12 * scale;
    const x = Math.cos(rad) * rLabel;
    const y = Math.sin(rad) * rLabel;
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(rad + Math.PI / 2);
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3 * scale;
    ctx.lineJoin = 'round';
    ctx.strokeText(String(Math.abs(a)), 0, 0);
    ctx.fillText(String(Math.abs(a)), 0, 0);
    ctx.restore();
  });

  ctx.rotate((clampedRoll * Math.PI) / 180);
  ctx.fillStyle = '#fd0000';
  ctx.strokeStyle = '#fd0000';
  ctx.lineWidth = 1.5 * scale;
  ctx.beginPath();
  ctx.moveTo(0, -(R - 30 * scale));
  ctx.lineTo(-8 * scale, -(R - 38 * scale));
  ctx.lineTo(8 * scale, -(R - 38 * scale));
  ctx.closePath();
  ctx.stroke();
  ctx.restore();

  ctx.save();
  ctx.translate(R, R);
  ctx.strokeStyle = '#e53935';
  ctx.lineWidth = 3 * scale;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(-65 * scale, 0);
  ctx.lineTo(-22 * scale, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(22 * scale, 0);
  ctx.lineTo(65 * scale, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(-22 * scale, 0);
  ctx.lineTo(-10 * scale, 8 * scale);
  ctx.lineTo(0, 0);
  ctx.lineTo(10 * scale, 8 * scale);
  ctx.lineTo(22 * scale, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.arc(0, 0, 4 * scale, 0, Math.PI * 2);
  ctx.fillStyle = '#e53935';
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(-R + 10 * scale, 0);
  ctx.lineTo(-R + 32 * scale, 0);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(R - 10 * scale, 0);
  ctx.lineTo(R - 32 * scale, 0);
  ctx.stroke();
  ctx.restore();
  ctx.restore();

  if (Math.abs(roll) > 60) {
    ctx.save();
    ctx.translate(R, R);
    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = 'rgba(0,0,0,0.8)';
    ctx.lineWidth = 3 * scale;
    ctx.font = `bold ${12 * scale}px sans-serif`;
    ctx.textAlign = 'center';
    const rollArrow = roll > 0 ? '►' : roll < 0 ? '◄' : '';
    ctx.strokeText(`${rollArrow} ${roll.toFixed(0)}°`, 0, R - 15 * scale);
    ctx.fillText(`${rollArrow} ${roll.toFixed(0)}°`, 0, R - 15 * scale);
    ctx.restore();
  }
}

function AttitudeIndicator({
  pitch,
  roll,
  size = 220,
}: {
  pitch: number;
  roll: number;
  size?: number;
}) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (canvasRef.current) {
      drawAI(canvasRef.current, pitch, roll);
    }
  }, [pitch, roll, size]);

  return (
    <canvas
      ref={canvasRef}
      width={size}
      height={size}
      className="attitude-indicator"
    />
  );
}

function lerp(a: number, b: number, t: number) {
  return Math.round(a + (b - a) * t);
}

function lerpColor(
  from: [number, number, number],
  to: [number, number, number],
  t: number
) {
  return `rgb(${lerp(from[0], to[0], t)}, ${lerp(from[1], to[1], t)}, ${lerp(from[2], to[2], t)})`;
}

const GRAY: [number, number, number] = [85, 85, 85]; // #555
const GREEN: [number, number, number] = [76, 175, 80]; // #4caf50
const RED: [number, number, number] = [229, 57, 53]; // #e53935

function getWheelColor(value: number) {
  const t = Math.min(Math.abs(value), 100) / 100; // 0..1
  if (value > 0) return lerpColor(GRAY, GREEN, t);
  if (value < 0) return lerpColor(GRAY, RED, t);
  return `rgb(${GRAY[0]}, ${GRAY[1]}, ${GRAY[2]})`;
}

function Wheel({ label, value }: { label: string; value: number }) {
  const color = getWheelColor(value);
  return (
    <div className="wheel">
      <div
        className="wheel__bar"
        style={{ '--wheel-color': color } as React.CSSProperties}
      >
        {value > 0 && <span className="wheel__arrow wheel__arrow--up" />}
        {(value < 0 || value == 0) && (
          <span style={{ borderTop: '7px solid #fff' }} />
        )}
        {Math.abs(value)}%
        {value < 0 && <span className="wheel__arrow wheel__arrow--down" />}
        {(value > 0 || value == 0) && (
          <span style={{ borderTop: '7px solid #fff' }} />
        )}
      </div>
      <span className="wheel__label">{label}</span>
    </div>
  );
}

function MyGyro() {
  const gyro = useGyroControls();
  const motors = useMotorControls();
  const isMobile = useIsMobile();

  return (
    <div className="gyro-panel">
      <div className="gyro-panel__block">
        <div className="gyro-panel__row">
          <div className="gyro-panel__wheels-col">
            <Wheel
              label="FL"
              value={motors.fl}
            />
            <Wheel
              label="RL"
              value={motors.rl}
            />
          </div>
          <AttitudeIndicator
            pitch={gyro.pitch}
            roll={gyro.roll}
            size={isMobile ? 220 : 270}
          />
          <div className="gyro-panel__wheels-col">
            <Wheel
              label="FR"
              value={motors.fr}
            />
            <Wheel
              label="RR"
              value={motors.rr}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
export default MyGyro;
