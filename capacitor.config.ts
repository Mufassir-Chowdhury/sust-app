import { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'edu.sust.app',
  appName: 'sust-app',
  webDir: 'build',
  server: {
    androidScheme: 'https'
  }
};

export default config;
