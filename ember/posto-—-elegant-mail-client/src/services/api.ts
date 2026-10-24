/**
 * POSTO Mail Service Layer
 */

import { User, Message, Recipient } from '../types';

const SAMPLE_TEMPLATES = [
  {
    sender: { id: 101, name: 'Eleanor Vance', email: 'eleanor.vance@studio.co', avatarColor: '#8C4E26' },
    subject: 'Autumn Editorial Campaign & Typography Specs',
    body: `Hello,\n\nI have reviewed the proofs for our upcoming autumn release. The warm linen and mocha palette feels exceptionally tactile and refined. Attached are the final vector assets and print specifications for your sign-off.\n\nCould you review by tomorrow afternoon?\n\nWarm regards,\nEleanor`,
    tag: 'Design',
    attachment: { name: 'Autumn_Editorial_Specs.pdf', size: '2.4 MB' },
    is_read: false,
    is_starred: true,
  },
  {
    sender: { id: 102, name: 'Marcus Sterling', email: 'm.sterling@foundry.io', avatarColor: '#3D2619' },
    subject: 'Architecture Review: Binary serialization & indexing milestones',
    body: `Hey,\n\nGreat progress on the file manager and fixed-capacity binary buffers. Everything is parsing with zero memory leaks. Let's do a quick sync at 2 PM to walk through the recipient chaining logic.\n\nBest,\nMarcus`,
    tag: 'Engineering',
    attachment: { name: 'Engine_Benchmark_v1.zip', size: '1.1 MB' },
    is_read: false,
    is_starred: false,
  },
  {
    sender: { id: 103, name: 'Clara Dubois', email: 'clara@atelier-postal.fr', avatarColor: '#A75427' },
    subject: 'Your custom brass wax seal stamp has been dispatched 📮',
    body: `Dear client,\n\nYour bespoke engraved seal with the Posto monogram has passed inspection. It is carefully packed with two sticks of Venetian terracotta sealing wax.\n\nTracking: POST-FR-98214\n\nThank you for choosing handmade quality!`,
    tag: 'Orders',
    attachment: { name: 'Dispatch_Receipt_98214.pdf', size: '420 KB' },
    is_read: true,
    is_starred: true,
  },
  {
    sender: { id: 104, name: 'David Zhao', email: 'david.zhao@venture.org', avatarColor: '#5A3824' },
    subject: 'Invitation: Private Salon & Exhibition Preview',
    body: `You are cordially invited to our private evening reception celebrating classic typography and physical printmaking. Drinks and curated tastings will be served from 7 PM onward.\n\nPlease RSVP by replying to this thread.\n\nDavid`,
    tag: 'Events',
    is_read: true,
    is_starred: false,
  },
  {
    sender: { id: 105, name: 'Sophia Larsson', email: 'sophia@nordicdesign.se', avatarColor: '#7A4828' },
    subject: 'Updated brand guidelines & color tokens for Posto',
    body: `Hi team,\n\nHere are the updated hex tokens for our warm sand, espresso, and terracotta scales. Please verify that all card radii and typography hierarchies align with the brand constitution.\n\nCheers,\nSophia`,
    tag: 'Design',
    attachment: { name: 'Posto_Design_Tokens.json', size: '18 KB' },
    is_read: true,
    is_starred: false,
  },
  {
    sender: { id: 106, name: 'The Posto Team', email: 'concierge@posto.mail', avatarColor: '#2C180E' },
    subject: 'Welcome to your private POSTO correspondence account',
    body: `Welcome!\n\nYour Posto mailbox is completely configured. Enjoy private, uncluttered mail management with custom wax seals, instant search, and refined stationery styling.\n\nFeel free to explore your inbox or draft your first message!`,
    tag: 'Welcome',
    is_read: true,
    is_starred: true,
  }
];

class MailService {
  private currentUser: User | null = null;
  private messages: Message[] = [];
  private nextMessageId: number = 2000;

  constructor() {
    // Clear old session so starting animation and login ALWAYS show!
    sessionStorage.removeItem('posto_session_initialized');
    this.currentUser = null;
  }

  private saveMessages(): void {
    localStorage.setItem('posto_messages_v3', JSON.stringify(this.messages));
  }

  public getCurrentUser(): User | null {
    return this.currentUser;
  }

  public async login(email: string, _password: string): Promise<User> {
    const cleanEmail = email.trim().toLowerCase();
    const name = cleanEmail.split('@')[0].replace(/[._]/g, ' ').replace(/\b\w/g, l => l.toUpperCase());

    const user: User = {
      id: Math.floor(Math.random() * 8000) + 1000,
      name: name || 'Alex Morgan',
      email: cleanEmail,
      created_at: Math.floor(Date.now() / 1000),
    };

    this.currentUser = user;
    this.seedUserMailbox(user);
    return user;
  }

  public async register(name: string, email: string, _password: string): Promise<User> {
    const cleanEmail = email.trim().toLowerCase();
    const user: User = {
      id: Math.floor(Math.random() * 8000) + 1000,
      name: name.trim() || 'New User',
      email: cleanEmail,
      created_at: Math.floor(Date.now() / 1000),
    };

    this.currentUser = user;
    this.seedUserMailbox(user);
    return user;
  }

  private seedUserMailbox(user: User): void {
    const now = Math.floor(Date.now() / 1000);
    this.messages = SAMPLE_TEMPLATES.map((tmpl, idx) => ({
      id: this.nextMessageId++,
      sender: tmpl.sender,
      recipients: [{ id: user.id, name: user.name, email: user.email, type: 'TO' as const }],
      subject: tmpl.subject,
      body: tmpl.body,
      created_at: now - (idx * 3600 * 4) - 900,
      is_read: tmpl.is_read,
      is_starred: tmpl.is_starred,
      is_deleted: false,
      tag: tmpl.tag,
      attachment: tmpl.attachment,
      wax_seal: 'monogram',
    }));
    this.saveMessages();
  }

  public async logout(): Promise<void> {
    this.currentUser = null;
  }

  public async getInbox(): Promise<Message[]> {
    if (!this.currentUser) return [];
    const myEmail = this.currentUser.email.toLowerCase();
    return this.messages.filter(m => 
      !m.is_deleted && 
      m.recipients.some(r => r.email.toLowerCase() === myEmail)
    );
  }

  public async getSent(): Promise<Message[]> {
    if (!this.currentUser) return [];
    const myEmail = this.currentUser.email.toLowerCase();
    return this.messages.filter(m => 
      !m.is_deleted && 
      m.sender.email.toLowerCase() === myEmail
    );
  }

  public async getStarred(): Promise<Message[]> {
    if (!this.currentUser) return [];
    const myEmail = this.currentUser.email.toLowerCase();
    return this.messages.filter(m => 
      !m.is_deleted && 
      m.is_starred &&
      (m.recipients.some(r => r.email.toLowerCase() === myEmail) || m.sender.email.toLowerCase() === myEmail)
    );
  }

  public async getTrash(): Promise<Message[]> {
    if (!this.currentUser) return [];
    const myEmail = this.currentUser.email.toLowerCase();
    return this.messages.filter(m => 
      m.is_deleted && 
      (m.recipients.some(r => r.email.toLowerCase() === myEmail) || m.sender.email.toLowerCase() === myEmail)
    );
  }

  public async sendMail(payload: {
    subject: string;
    body: string;
    recipients: { email: string; name?: string; type: 'TO' | 'CC' | 'BCC' }[];
    tag?: string;
    attachment?: { name: string; size: string };
    wax_seal?: 'monogram' | 'oak' | 'crest' | 'star';
  }): Promise<Message> {
    if (!this.currentUser) {
      throw new Error('Must be logged in to send mail');
    }

    const formattedRecipients: Recipient[] = payload.recipients.map((r, idx) => ({
      id: Math.floor(Math.random() * 900) + 50,
      name: r.name || r.email.split('@')[0],
      email: r.email.trim(),
      type: r.type || 'TO'
    }));

    const newMsg: Message = {
      id: this.nextMessageId++,
      sender: {
        id: this.currentUser.id,
        name: this.currentUser.name,
        email: this.currentUser.email,
        avatarColor: '#C8622A'
      },
      recipients: formattedRecipients,
      subject: payload.subject.trim() || '(No Subject)',
      body: payload.body || '',
      created_at: Math.floor(Date.now() / 1000),
      is_read: true,
      is_starred: false,
      is_deleted: false,
      tag: payload.tag || 'Outgoing',
      attachment: payload.attachment,
      wax_seal: payload.wax_seal || 'monogram',
    };

    this.messages.unshift(newMsg);
    this.saveMessages();
    return newMsg;
  }

  public async markAsRead(id: number, isRead: boolean = true): Promise<boolean> {
    const target = this.messages.find(m => m.id === id);
    if (target) {
      target.is_read = isRead;
      this.saveMessages();
      return true;
    }
    return false;
  }

  public async toggleStar(id: number): Promise<boolean> {
    const target = this.messages.find(m => m.id === id);
    if (!target) return false;
    target.is_starred = !target.is_starred;
    this.saveMessages();
    return target.is_starred;
  }

  public async deleteMail(id: number): Promise<boolean> {
    const target = this.messages.find(m => m.id === id);
    if (target) {
      target.is_deleted = true;
      this.saveMessages();
      return true;
    }
    return false;
  }

  public async restoreMail(id: number): Promise<boolean> {
    const target = this.messages.find(m => m.id === id);
    if (target) {
      target.is_deleted = false;
      this.saveMessages();
      return true;
    }
    return false;
  }

  public async permanentDelete(id: number): Promise<boolean> {
    this.messages = this.messages.filter(m => m.id !== id);
    this.saveMessages();
    return true;
  }
}

export const mailService = new MailService();
