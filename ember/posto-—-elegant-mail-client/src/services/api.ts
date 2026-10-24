/**
 * Live POSTO -> EMBER -> FORGE service.
 * The browser never reads FORGE storage files. Unsupported mailbox-state features fail honestly.
 */
import { User, Message, Recipient } from '../types';
import { request, setToken, ApiError } from './httpClient';

type WireUser = { id: number; name: string; email: string; created_at?: number };
type WireMessage = { id: number; sender_id: number; subject: string; body: string; created_at: number };
type QueryResult = { rows: Record<string, string | number>[]; row_count: number };

class MailService {
  private currentUser: User | null = this.readSavedUser();

  private readSavedUser(): User | null {
    try {
      const raw = sessionStorage.getItem('ember_current_user');
      return raw ? JSON.parse(raw) as User : null;
    } catch { return null; }
  }
  private saveUser(user: User | null): void {
    this.currentUser = user;
    if (user) sessionStorage.setItem('ember_current_user', JSON.stringify(user));
    else sessionStorage.removeItem('ember_current_user');
  }
  public getCurrentUser(): User | null { return this.currentUser; }

  public async login(email: string, password: string): Promise<User> {
    const result = await request<{token: string}>('/auth/login', {
      method: 'POST', body: JSON.stringify({ email: email.trim(), password })
    });
    if (!result.token) throw new Error('FORGE did not return an authentication token.');
    setToken(result.token);
    const user = await request<WireUser>('/users/me');
    const mapped: User = { id: user.id, name: user.name, email: user.email, created_at: user.created_at ?? Math.floor(Date.now() / 1000) };
    this.saveUser(mapped);
    return mapped;
  }
  public async register(name: string, email: string, password: string): Promise<User> {
    await request<WireUser>('/users', { method: 'POST', body: JSON.stringify({ name: name.trim(), email: email.trim(), password }) });
    return this.login(email, password);
  }
  public async logout(): Promise<void> {
    try { await request('/auth/logout', { method: 'POST', body: '{}' }); }
    finally { setToken(null); this.saveUser(null); }
  }

  private async loadMessages(): Promise<Message[]> {
    if (!this.currentUser) return [];
    const [wireMessages, query] = await Promise.all([
      request<WireMessage[]>('/messages'),
      request<QueryResult>('/query', { method: 'POST', body: JSON.stringify({ query: 'SELECT message_id, user_id, type FROM recipients;' }) })
    ]);
    const rows = query.rows ?? [];
    const recipientRows = rows.map(r => ({
      message_id: Number(r.message_id), user_id: Number(r.user_id), type: String(r.type || 'TO') as Recipient['type']
    }));
    return wireMessages.map((m) => {
      const recipients: Recipient[] = recipientRows.filter(r => r.message_id === m.id).map(r => ({
        id: r.user_id, name: r.user_id === this.currentUser!.id ? this.currentUser!.name : `User #${r.user_id}`,
        email: r.user_id === this.currentUser!.id ? this.currentUser!.email : '', type: r.type
      }));
      const isOwn = m.sender_id === this.currentUser!.id;
      return {
        id: m.id,
        sender: { id: m.sender_id, name: isOwn ? this.currentUser!.name : `User #${m.sender_id}`, email: isOwn ? this.currentUser!.email : '' },
        recipients, subject: m.subject, body: m.body, created_at: Number(m.created_at),
        is_read: true, is_starred: false, is_deleted: false
      };
    });
  }
  public async getInbox(): Promise<Message[]> {
    const list = await this.loadMessages();
    return list.filter(m => m.sender.id !== this.currentUser?.id && m.recipients.some(r => r.id === this.currentUser?.id));
  }
  public async getSent(): Promise<Message[]> {
    return (await this.loadMessages()).filter(m => m.sender.id === this.currentUser?.id);
  }
  public async getStarred(): Promise<Message[]> {
    throw new ApiError('Starred mail requires per-user mailbox state in FORGE and is not available yet.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async getTrash(): Promise<Message[]> {
    throw new ApiError('Trash requires per-user mailbox state in FORGE and is not available yet.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async sendMail(payload: {
    subject: string; body: string;
    recipients: { email: string; name?: string; type: 'TO' | 'CC' | 'BCC' }[];
    tag?: string; attachment?: { name: string; size: string };
    wax_seal?: 'monogram' | 'oak' | 'crest' | 'star';
  }): Promise<Message> {
    if (!this.currentUser) throw new Error('Please sign in before sending mail.');
    if (payload.attachment) throw new ApiError('Attachments are not supported by the current FORGE API.', 501, 'ATTACHMENTS_NOT_SUPPORTED');
    const recipients = await Promise.all(payload.recipients.map(async recipient => {
      const user = await request<WireUser>(`/users/lookup?email=${encodeURIComponent(recipient.email.trim())}`);
      return { user_id: user.id, type: recipient.type };
    }));
    const created = await request<{id: number}>('/messages', { method: 'POST', body: JSON.stringify({
      subject: payload.subject.trim() || '(No Subject)', body: payload.body, recipients
    })});
    const wire = await request<WireMessage>(`/messages/${created.id}`);
    return {
      id: wire.id,
      sender: { id: this.currentUser.id, name: this.currentUser.name, email: this.currentUser.email },
      recipients: payload.recipients.map((r, i) => ({ id: recipients[i].user_id, name: r.name || r.email, email: r.email, type: r.type })),
      subject: wire.subject, body: wire.body, created_at: Number(wire.created_at),
      is_read: true, is_starred: false, is_deleted: false
    };
  }
  public async markAsRead(_id: number, _isRead = true): Promise<boolean> {
    throw new ApiError('Read/unread state is not yet stored per user in FORGE.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async toggleStar(_id: number): Promise<boolean> {
    throw new ApiError('Starred state is not yet stored per user in FORGE.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async deleteMail(_id: number): Promise<boolean> {
    throw new ApiError('Recipient-specific trash is not yet implemented in FORGE.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async restoreMail(_id: number): Promise<boolean> {
    throw new ApiError('Restore is not yet implemented in FORGE.', 501, 'FEATURE_NOT_SUPPORTED');
  }
  public async permanentDelete(_id: number): Promise<boolean> {
    throw new ApiError('Permanent deletion is not yet implemented in FORGE.', 501, 'FEATURE_NOT_SUPPORTED');
  }
}
export const mailService = new MailService();
