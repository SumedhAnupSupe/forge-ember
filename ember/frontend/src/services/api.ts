import type { Message, User } from '../types';

const API = '/api/v1';
let token: string | null = null;
export function setToken(value: string | null) { token = value; }

async function request<T>(path: string, init: RequestInit = {}): Promise<T> {
  const headers = new Headers(init.headers);
  headers.set('Accept', 'application/json');
  if (init.body) headers.set('Content-Type', 'application/json');
  if (token) headers.set('Authorization', `Bearer ${token}`);
  const response = await fetch(`${API}${path}`, { ...init, headers });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload.error || `Request failed (${response.status})`);
  return payload as T;
}
export const api = {
  async login(email: string, password: string) {
    const result = await request<{ token: string; user_id: number }>('/auth/login', { method: 'POST', body: JSON.stringify({ email, password }) });
    setToken(result.token);
    const user = await request<User>('/auth/me');
    return user;
  },
  async register(name: string, email: string, password: string) {
    await request('/auth/register', { method: 'POST', body: JSON.stringify({ name, email, password }) });
    return this.login(email, password);
  },
  async logout() { try { await request('/auth/logout', { method: 'POST', body: '{}' }); } finally { setToken(null); } },
  me: () => request<User>('/auth/me'),
  messages: () => request<Message[]>('/messages'),
  message: (id: number) => request<Message>(`/messages/${id}`),
  send: (subject: string, body: string, recipients: { email: string; type: 'TO' | 'CC' | 'BCC' }[]) =>
    request<Message>('/messages', { method: 'POST', body: JSON.stringify({ subject, body, recipients }) }),
  delete: (id: number) => request(`/messages/${id}`, { method: 'DELETE' })
};
