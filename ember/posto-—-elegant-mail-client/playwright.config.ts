import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: './tests/e2e',
  fullyParallel: false,
  retries: 0,
  reporter: 'list',
  use: {
    baseURL: 'http://127.0.0.1:3000',
    trace: 'retain-on-failure',
    ...devices['Desktop Chrome'],
  },
  webServer: [
    {
      command: '../../build/forge/forge-server --host 127.0.0.1 --port 8080 --data-dir ../../build/e2e-forge-data',
      url: 'http://127.0.0.1:8080/health',
      reuseExistingServer: !process.env.CI,
      timeout: 30000,
    },
    {
      command: '../../build/ember/ember-server',
      url: 'http://127.0.0.1:8081/health',
      reuseExistingServer: !process.env.CI,
      timeout: 30000,
      env: { FORGE_ORIGIN: 'http://127.0.0.1:8080', EMBER_HOST: '127.0.0.1', EMBER_PORT: '8081' },
    },
    {
      command: 'npm run dev -- --host 127.0.0.1',
      url: 'http://127.0.0.1:3000',
      reuseExistingServer: !process.env.CI,
      timeout: 30000,
    },
  ],
});
