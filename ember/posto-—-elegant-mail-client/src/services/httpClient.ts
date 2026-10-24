const API_BASE = (import.meta.env.VITE_EMBER_API_URL || '').replace(/\/$/, '');

export class ApiError extends Error {
  status: number;
  code: string;
  constructor(message: string, status: number, code = 'API_ERROR') {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.code = code;
  }
}

export function getToken(): string | null {
  return sessionStorage.getItem('ember_access_token');
}
export function setToken(token: string | null): void {
  if (token) sessionStorage.setItem('ember_access_token', token);
  else sessionStorage.removeItem('ember_access_token');
}

export async function request<T>(path: string, init: RequestInit = {}): Promise<T> {
  const headers = new Headers(init.headers);
  headers.set('Accept', 'application/json');
  if (init.body && !headers.has('Content-Type')) headers.set('Content-Type', 'application/json');
  const token = getToken();
  if (token) headers.set('Authorization', `Bearer ${token}`);
  let response: Response;
  try {
    response = await fetch(`${API_BASE}/api/v1${path}`, { ...init, headers });
  } catch {
    throw new ApiError('EMBER is unavailable. Start the backend and try again.', 0, 'NETWORK_ERROR');
  }
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const message = payload?.error?.message || payload?.error || payload?.message || `Request failed (${response.status})`;
    throw new ApiError(String(message), response.status, payload?.error?.code || 'API_ERROR');
  }
  return payload as T;
}
