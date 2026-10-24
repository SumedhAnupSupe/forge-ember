/**
 * Live POSTO API adapter. No sample accounts or browser-persisted messages.
 * The server-side FORGE database is the source of truth.
 */
import type { Message, Recipient, User } from "../types";

const API_BASE = (import.meta.env.VITE_API_BASE_URL || "/api").replace(/\/$/, "");
const TOKEN_KEY = "posto_access_token";
const USER_KEY = "posto_current_user";

type ApiErrorBody = { error?: { code?: string; message?: string }; detail?: string };
async function request<T>(path: string, init: RequestInit = {}): Promise<T> {
  const token = localStorage.getItem(TOKEN_KEY);
  const headers = new Headers(init.headers);
  headers.set("Accept", "application/json");
  if (init.body) headers.set("Content-Type", "application/json");
  if (token) headers.set("Authorization", `Bearer ${token}`);
  let response: Response;
  try {
    response = await fetch(`${API_BASE}${path}`, { ...init, headers });
  } catch {
    throw new Error("Cannot reach EMBER. Check that FastAPI is running on port 8081.");
  }
  const payload = response.status === 204 ? undefined : await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = payload as ApiErrorBody;
    if (response.status === 401) {
      localStorage.removeItem(TOKEN_KEY);
      localStorage.removeItem(USER_KEY);
    }
    throw new Error(error.error?.message || error.detail || `Request failed (${response.status})`);
  }
  return payload as T;
}

type BackendMessage = {
  id: number; sender_id: number;
  sender: { id: number; name: string; email: string };
  recipients: { id: number; name: string; email: string; type: "TO" | "CC" | "BCC" }[];
  subject: string; body: string; created_at: number;
  is_read?: boolean; is_starred?: boolean; is_deleted?: boolean;
};
type UserResponse = User;
function toMessage(m: BackendMessage): Message {
  return {
    id: m.id,
    sender: { ...m.sender, avatarColor: "#8C4E26" },
    recipients: m.recipients.map((r): Recipient => ({ ...r })),
    subject: m.subject,
    body: m.body,
    created_at: m.created_at,
    is_read: Boolean(m.is_read),
    is_starred: Boolean(m.is_starred),
    is_deleted: Boolean(m.is_deleted),
  };
}

class MailService {
  private currentUser: User | null = this.restoreUser();

  private restoreUser(): User | null {
    try {
      const raw = localStorage.getItem(USER_KEY);
      return raw ? JSON.parse(raw) as User : null;
    } catch {
      localStorage.removeItem(USER_KEY);
      return null;
    }
  }

  private setSession(token: string, user: User): User {
    localStorage.setItem(TOKEN_KEY, token);
    localStorage.setItem(USER_KEY, JSON.stringify(user));
    this.currentUser = user;
    return user;
  }

  public getCurrentUser(): User | null {
    return this.currentUser;
  }

  public async login(email: string, password: string): Promise<User> {
    const result = await request<{ token: string; user: User }>("/v1/auth/login", {
      method: "POST", body: JSON.stringify({ email: email.trim().toLowerCase(), password }),
    });
    return this.setSession(result.token, result.user);
  }

  public async register(name: string, email: string, password: string): Promise<User> {
    await request<UserResponse>("/v1/auth/register", {
      method: "POST",
      body: JSON.stringify({ name: name.trim(), email: email.trim().toLowerCase(), password }),
    });
    // Registration does not automatically sign in; authenticate with the newly-created account.
    return this.login(email, password);
  }

  public async logout(): Promise<void> {
    try {
      if (localStorage.getItem(TOKEN_KEY)) await request("/v1/auth/logout", { method: "POST" });
    } finally {
      localStorage.removeItem(TOKEN_KEY);
      localStorage.removeItem(USER_KEY);
      this.currentUser = null;
    }
  }

  public async getInbox(): Promise<Message[]> {
    const data = await request<BackendMessage[]>("/v1/mailboxes/inbox");
    return data.map(toMessage);
  }

  public async getSent(): Promise<Message[]> {
    const data = await request<BackendMessage[]>("/v1/mailboxes/sent");
    return data.map(toMessage);
  }

  // FORGE does not yet persist per-user starred/trash/read state. Never fake it locally.
  public async getStarred(): Promise<Message[]> { return []; }
  public async getTrash(): Promise<Message[]> { return []; }
  public async markAsRead(_id: number, _isRead = true): Promise<boolean> { return false; }
  public async toggleStar(_id: number): Promise<boolean> { return false; }
  public async restoreMail(_id: number): Promise<boolean> { return false; }

  public async sendMail(payload: {
    subject: string; body: string;
    recipients: { email: string; name?: string; type: "TO" | "CC" | "BCC" }[];
    tag?: string; attachment?: { name: string; size: string };
    wax_seal?: "monogram" | "oak" | "crest" | "star";
  }): Promise<Message> {
    if (!this.currentUser || !localStorage.getItem(TOKEN_KEY)) {
      throw new Error("Sign in before sending mail.");
    }
    const recipients: { user_id: number; type: "TO" | "CC" | "BCC" }[] = [];
    for (const recipient of payload.recipients) {
      const user = await request<UserResponse>(`/v1/users/lookup?email=${encodeURIComponent(recipient.email.trim().toLowerCase())}`);
      if (user.id === this.currentUser.id) throw new Error("You cannot send mail to your own account yet.");
      recipients.push({ user_id: user.id, type: recipient.type });
    }
    const created = await request<{ id: number }>("/v1/messages", {
      method: "POST",
      body: JSON.stringify({ subject: payload.subject.trim(), body: payload.body, recipients }),
    });
    // Return a minimal message only after FORGE confirms persistence; reload obtains canonical details.
    return {
      id: created.id,
      sender: { ...this.currentUser, avatarColor: "#C8622A" },
      recipients: payload.recipients.map((r, i) => ({
        id: recipients[i].user_id, name: r.name || r.email.split("@")[0],
        email: r.email.trim(), type: r.type,
      })),
      subject: payload.subject.trim(), body: payload.body,
      created_at: Math.floor(Date.now() / 1000),
      is_read: true, is_starred: false, is_deleted: false,
    };
  }

  public async deleteMail(id: number): Promise<boolean> {
    await request(`/v1/messages/${id}`, { method: "DELETE" });
    return true;
  }

  public async permanentDelete(id: number): Promise<boolean> {
    return this.deleteMail(id);
  }
}

export const mailService = new MailService();
