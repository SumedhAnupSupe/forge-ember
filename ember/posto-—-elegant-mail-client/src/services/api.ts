/**
 * POSTO API service. Persistent state lives in FORGE, reached through FastAPI.
 */
import { User, Message, RecipientType } from '../types';

const API_BASE = (import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:8000/api/v1').replace(/\/$/, '');
const TOKEN_KEY = 'posto_access_token';
const USER_KEY = 'posto_current_user';
type AuthResponse = { user: User; token: string };

class MailService {
  private token: string | null = localStorage.getItem(TOKEN_KEY);
  private currentUser: User | null = this.readUser();

  private readUser(): User | null {
    try {
      const value = localStorage.getItem(USER_KEY);
      return value ? JSON.parse(value) as User : null;
    } catch { return null; }
  }

  private saveSession(token: string, user: User): void {
    this.token = token;
    this.currentUser = user;
    localStorage.setItem(TOKEN_KEY, token);
    localStorage.setItem(USER_KEY, JSON.stringify(user));
  }

  private async request<T>(path: string, init: RequestInit = {}): Promise<T> {
    const headers = new Headers(init.headers);
    headers.set('Content-Type', 'application/json');
    if (this.token) headers.set('Authorization', 'Bearer ' + this.token);
    let response: Response;
    try {
      response = await fetch(API_BASE + path, { ...init, headers });
    } catch {
      throw new Error('Cannot reach EMBER. Start FORGE and the FastAPI backend, then retry.');
    }
    const data = await response.json().catch(() => ({}));
    if (!response.ok) {
      const message = data?.error?.message || data?.detail || data?.error || 'Request failed';
      if (response.status === 401) this.clearSession();
      throw new Error(String(message));
    }
    return data as T;
  }

  private clearSession(): void {
    this.token = null;
    this.currentUser = null;
    localStorage.removeItem(TOKEN_KEY);
    localStorage.removeItem(USER_KEY);
  }

  public getCurrentUser(): User | null { return this.currentUser; }

  public async login(email: string, password: string): Promise<User> {
    const result = await this.request<AuthResponse>('/auth/login', {
      method: 'POST', body: JSON.stringify({ email: email.trim().toLowerCase(), password })
    });
    this.saveSession(result.token, result.user);
    return result.user;
  }

  public async register(name: string, email: string, password: string): Promise<User> {
    const result = await this.request<AuthResponse>('/auth/register', {
      method: 'POST', body: JSON.stringify({ name: name.trim(), email: email.trim().toLowerCase(), password })
    });
    this.saveSession(result.token, result.user);
    return result.user;
  }

  public async logout(): Promise<void> {
    try {
      if (this.token) await this.request('/auth/logout', { method: 'POST' });
    } finally { this.clearSession(); }
  }

  private list(folder: string): Promise<Message[]> {
    return this.request<Message[]>(`/mail/${folder}?sort=newest`);
  }
  public getInbox(): Promise<Message[]> { return this.list('inbox'); }
  public getSent(): Promise<Message[]> { return this.list('sent'); }
  public getStarred(): Promise<Message[]> { return this.list('starred'); }
  public getTrash(): Promise<Message[]> { return this.list('trash'); }

  public async sendMail(payload: {
    subject: string; body: string;
    recipients: { email: string; name?: string; type: RecipientType }[];
    tag?: string; attachment?: { name: string; size: string };
    wax_seal?: 'monogram' | 'oak' | 'crest' | 'star';
  }): Promise<Message> {
    const created = await this.request<{ success: boolean; id: number }>('/mail', {
      method: 'POST',
      body: JSON.stringify({
        subject: payload.subject.trim(), body: payload.body,
        recipients: payload.recipients.map(r => ({ email: r.email.trim(), type: r.type || 'TO' }))
      })
    });
    return this.request<Message>(`/mail/item/${created.id}`);
  }

  public async markAsRead(id: number, isRead = true): Promise<boolean> {
    await this.request(`/mail/${id}/read`, { method: 'PATCH', body: JSON.stringify({ value: isRead }) });
    return true;
  }

  public async toggleStar(id: number): Promise<boolean> {
    const message = await this.request<Message>(`/mail/item/${id}`);
    const value = !message.is_starred;
    await this.request(`/mail/${id}/star`, { method: 'PATCH', body: JSON.stringify({ value }) });
    return value;
  }

  public async deleteMail(id: number): Promise<boolean> {
    await this.request(`/mail/${id}`, { method: 'DELETE' }); return true;
  }
  public async restoreMail(id: number): Promise<boolean> {
    await this.request(`/mail/${id}/restore`, { method: 'POST' }); return true;
  }
  public async permanentDelete(id: number): Promise<boolean> {
    await this.request(`/mail/${id}/permanent`, { method: 'DELETE' }); return true;
  }
}

export const mailService = new MailService();
