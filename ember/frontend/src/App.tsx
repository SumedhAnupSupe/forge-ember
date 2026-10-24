import { FormEvent, useEffect, useMemo, useState } from 'react';
import { Inbox, Send, LogOut, MailPlus, Search, RefreshCw, Trash2, ArrowUpRight, Mail } from 'lucide-react';
import { api } from './services/api';
import type { Message, User } from './types';

type Folder = 'inbox' | 'sent';
export default function App() {
  const [user, setUser] = useState<User | null>(null);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [signup, setSignup] = useState(false);
  const [messages, setMessages] = useState<Message[]>([]);
  const [folder, setFolder] = useState<Folder>('inbox');
  const [query, setQuery] = useState('');
  const [selected, setSelected] = useState<Message | null>(null);
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);
  const [compose, setCompose] = useState(false);
  const [to, setTo] = useState('');
  const [subject, setSubject] = useState('');
  const [body, setBody] = useState('');

  async function refresh() {
    setBusy(true); setError('');
    try { setMessages(await api.messages()); }
    catch (e) { setError(e instanceof Error ? e.message : 'Unable to load mail'); }
    finally { setBusy(false); }
  }
  useEffect(() => { if (user) void refresh(); }, [user]);
  const visible = useMemo(() => messages.filter(m => {
    const matches = `${m.subject} ${m.body} ${m.sender?.name ?? ''} ${m.sender?.email ?? ''}`.toLowerCase().includes(query.toLowerCase());
    return matches && (folder === 'sent' ? m.sender_id === user?.id || m.sender?.id === user?.id : true);
  }), [messages, query, folder, user]);

  async function authenticate(e: FormEvent) {
    e.preventDefault(); setError(''); setBusy(true);
    try { setUser(signup ? await api.register(name, email, password) : await api.login(email, password)); }
    catch (e) { setError(e instanceof Error ? e.message : 'Authentication failed'); }
    finally { setBusy(false); }
  }
  async function send(e: FormEvent) {
    e.preventDefault(); setError('');
    try { await api.send(subject, body, to.split(',').map(value => ({ user_id: Number(value.trim()), type: 'TO' as const })).filter(r => Number.isInteger(r.user_id) && r.user_id > 0)); setCompose(false); setTo(''); setSubject(''); setBody(''); await refresh(); }
    catch (e) { setError(e instanceof Error ? e.message : 'Could not send message'); }
  }
  async function logout() { try { await api.logout(); } finally { setUser(null); setMessages([]); setSelected(null); } }

  if (!user) return <main className="auth-wrap"><section className="auth-card"><div className="brand-mark">e</div><p className="eyebrow">YOUR PRIVATE CORRESPONDENCE</p><h1>Mail, with a little more <em>meaning.</em></h1><p className="muted">A considered space for your conversations.</p><form onSubmit={authenticate} className="auth-form">{signup && <label>Your name<input required value={name} onChange={e => setName(e.target.value)} placeholder="Alex Morgan" /></label>}<label>Email address<input type="email" required value={email} onChange={e => setEmail(e.target.value)} placeholder="you@example.com" /></label><label>Password<input type="password" required minLength={8} value={password} onChange={e => setPassword(e.target.value)} placeholder="At least 8 characters" /></label>{error && <p className="error">{error}</p>}<button className="primary" disabled={busy}>{busy ? 'Please wait…' : signup ? 'Create account' : 'Sign in'} <ArrowUpRight size={16}/></button></form><button className="link-button" onClick={() => { setSignup(!signup); setError(''); }}>{signup ? 'Already have an account? Sign in' : 'New to EMBER? Create an account'}</button><p className="fine-print">Powered by EMBER + FORGE</p></section></main>;

  return <div className="app-shell"><aside className="sidebar"><div className="brand-row"><div className="brand-mark small">e</div><span>ember<span className="brand-dot">.</span></span></div><button className="compose-button" onClick={() => setCompose(true)}><MailPlus size={17}/> Compose</button><p className="nav-label">MAILBOX</p><button className={folder === 'inbox' ? 'nav-item active' : 'nav-item'} onClick={() => { setFolder('inbox'); setSelected(null); }}><Inbox size={17}/> Inbox <span>{messages.length}</span></button><button className={folder === 'sent' ? 'nav-item active' : 'nav-item'} onClick={() => { setFolder('sent'); setSelected(null); }}><Send size={17}/> Sent</button><div className="sidebar-bottom"><div className="avatar">{user.name.slice(0,1).toUpperCase()}</div><div className="user-meta"><b>{user.name}</b><small>{user.email}</small></div><button className="icon-button" title="Sign out" onClick={() => void logout()}><LogOut size={16}/></button></div></aside><main className="workspace"><header className="topbar"><div><p className="eyebrow">YOUR MAILBOX</p><h1>{folder === 'inbox' ? 'Inbox' : 'Sent mail'}<span className="count">{visible.length}</span></h1></div><div className="top-actions"><label className="search"><Search size={16}/><input value={query} onChange={e => setQuery(e.target.value)} placeholder="Search mail…" /></label><button className="icon-button" title="Refresh" onClick={() => void refresh()}><RefreshCw size={17}/></button></div></header>{error && <div className="notice">{error}</div>}<div className="mail-layout"><section className="mail-list">{busy ? <p className="empty">Loading your correspondence…</p> : visible.length === 0 ? <div className="empty-state"><Mail size={28}/><h3>Nothing here yet</h3><p>Your real messages from FORGE will appear here.</p></div> : visible.map(m => <button className={selected?.id === m.id ? 'message-row chosen' : 'message-row'} key={m.id} onClick={() => setSelected(m)}><div className="sender-avatar">{(m.sender?.name || m.sender?.email || '?').slice(0,1).toUpperCase()}</div><div className="message-summary"><div className="message-top"><b>{m.sender?.name || m.sender?.email || (m.sender_id === user.id ? 'You' : 'Message')}</b><time>{m.created_at ? new Date(m.created_at * 1000).toLocaleDateString() : ''}</time></div><strong>{m.subject || '(No subject)'}</strong><p>{(m.body || '').slice(0,100)}</p></div></button>)}</section><section className="reading-pane">{selected ? <><div className="reading-toolbar"><span className="eyebrow">MESSAGE DETAILS</span><button className="icon-button" title="Delete" onClick={async () => { try { await api.delete(selected.id); setSelected(null); await refresh(); } catch (e) { setError(e instanceof Error ? e.message : 'Delete failed'); } }}><Trash2 size={17}/></button></div><h2>{selected.subject || '(No subject)'}</h2><p className="muted">From {selected.sender?.name || selected.sender?.email || 'Unknown sender'}</p><div className="rule"/><article className="message-body">{selected.body}</article></> : <div className="reading-empty"><Mail size={35}/><p>Select a message to read it</p></div>}</section></div></main>{compose && <div className="modal-backdrop" onClick={() => setCompose(false)}><form className="compose-modal" onClick={e => e.stopPropagation()} onSubmit={send}><div className="modal-heading"><h2>New message</h2><button type="button" className="icon-button" onClick={() => setCompose(false)}>×</button></div><label>Recipient user IDs<input required value={to} onChange={e => setTo(e.target.value)} placeholder="Recipient user IDs, comma-separated (e.g. 2, 3)" /></label><label>Subject<input value={subject} onChange={e => setSubject(e.target.value)} placeholder="A thoughtful subject" /></label><label>Message<textarea required rows={9} value={body} onChange={e => setBody(e.target.value)} placeholder="Write your message…" /></label>{error && <p className="error">{error}</p>}<button className="primary">Send message <Send size={16}/></button></form></div>}</div>;
}
