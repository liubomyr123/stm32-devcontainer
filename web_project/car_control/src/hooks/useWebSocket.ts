import { useEffect, useRef } from 'react';
import { WebSocketManager } from '../class/WebSocketManager';

export function useWebSocket() {
  const manager = useRef(WebSocketManager.getInstance());

  useEffect(() => {
    manager.current?.connect();
  }, []);

  return {
    send: (data: string) => manager.current?.send(data),
    onMessage: (cb: (data: string) => void) => manager.current?.onMessage(cb),
  };
}
