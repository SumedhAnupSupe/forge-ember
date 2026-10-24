import type { ApiUser } from '../types/api';
import { request, setToken } from './httpClient';

interface LoginResponse { token: string; user_id: number; }
export const authService = {
  async login(email: string, password: string): Promise<ApiUser> {
    const result = await request<LoginResponse>('/auth/login', { method: 'POST', body: JSON.stringify({ email, password }) });
    if (!result.token) throw new Error('FORGE did not return a session token.');
    setToken(result.token);
    return request<ApiUser>('/users/me');
  },
  async register(name: string, email: string, password: string): Promise<ApiUser> {
    await request<ApiUser>('/users', { method: 'POST', body: JSON.stringify({ name, email, password }) });
    return this.login(email, password);
  },
  async me(): Promise<ApiUser | null> {
    if (!sessionStorage.getItem('ember_access_token')) return null;
    try { return await request<ApiUser>('/users/me'); }
    catch { setToken(null); return null; }
  },
  async logout(): Promise<void> {
    try { await request('/auth/logout', { method: 'POST', body: '{}' }); }
    finally { setToken(null); }
  },
};
