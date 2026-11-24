import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const backendPort = process.env.BACKEND_PORT || '8080';
const frontendPort = Number(process.env.FRONTEND_PORT) || 5173;

export default defineConfig({
  plugins: [react()],
  server: {
    port: frontendPort,
    host: 'localhost',
    proxy: {
      '/api': {
        target: `http://127.0.0.1:${backendPort}`,
        changeOrigin: true,
      }
    }
  }
});
