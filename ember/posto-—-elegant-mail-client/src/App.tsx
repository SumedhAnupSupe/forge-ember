import React, { useState, useEffect } from 'react';
import {
  Inbox,
  Send,
  Star,
  Mail,
  Search,
  Plus,
  Trash2,
  MailOpen,
  ArrowUpDown,
  RefreshCw,
  LogOut,
  X,
  Sparkles,
  ArrowRight,
  ArrowLeft,
  Paperclip,
  PanelLeftClose,
  PanelLeftOpen,
  Eye,
  EyeOff,
  Download,
  Reply,
  Stamp,
  CheckCircle2
} from 'lucide-react';
import { Logo } from './components/Logo';
import { PostalAnimation } from './components/PostalAnimation';
import { SendFlightAnimation } from './components/SendFlightAnimation';
import { mailService } from './services/api';
import { Message, User } from './types';

type ActiveFolder = 'inbox' | 'starred' | 'sent' | 'trash';
type FilterCategory = 'all' | 'unread' | 'starred' | 'attachments';

export default function App() {
  // Step 1: Starting Animation (runs initially, and whenever user clicks Refresh)
  const [showIntroAnimation, setShowIntroAnimation] = useState<boolean>(true);

  // Step 2: Authentication State
  const [currentUser, setCurrentUser] = useState<User | null>(mailService.getCurrentUser());
  const [authTab, setAuthTab] = useState<'login' | 'signup'>('login');
  const [authName, setAuthName] = useState<string>('');
  const [authEmail, setAuthEmail] = useState<string>('');
  const [authPassword, setAuthPassword] = useState<string>('');
  const [authError, setAuthError] = useState<string | null>(null);

  // Screen Controls: Option to close/open sidebar and inbox list
  const [isSidebarOpen, setIsSidebarOpen] = useState<boolean>(true);
  const [isInboxListOpen, setIsInboxListOpen] = useState<boolean>(true);

  // Mailbox State
  const [activeFolder, setActiveFolder] = useState<ActiveFolder>('inbox');
  const [activeCategory, setActiveCategory] = useState<FilterCategory>('all');
  const [messages, setMessages] = useState<Message[]>([]);
  const [selectedMail, setSelectedMail] = useState<Message | null>(null);
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [sortOrder, setSortOrder] = useState<'newest' | 'oldest'>('newest');
  const [isLoading, setIsLoading] = useState<boolean>(false);

  // Quick Reply
  const [replyText, setReplyText] = useState<string>('');
  const [isReplying, setIsReplying] = useState<boolean>(false);

  // Counts
  const [counts, setCounts] = useState({
    inbox: 0,
    starred: 0,
    sent: 0,
    trash: 0,
  });

  // Compose Modal State
  const [isComposeOpen, setIsComposeOpen] = useState<boolean>(false);
  const [composeTo, setComposeTo] = useState<string>('');
  const [composeSubject, setComposeSubject] = useState<string>('');
  const [composeBody, setComposeBody] = useState<string>('');
  const [composeTag, setComposeTag] = useState<string>('General');

  // Interactive Send Mail Flight Animation State
  const [activeFlight, setActiveFlight] = useState<{
    senderName: string;
    senderEmail: string;
    recipientEmail: string;
    subject: string;
    body: string;
    tag: string;
  } | null>(null);

  // Toast Notification
  const [toast, setToast] = useState<string | null>(null);

  const showToast = (msg: string) => {
    setToast(msg);
    setTimeout(() => setToast(null), 3200);
  };

  // Full Refresh Handler: Replays the starting animation and reloads everything
  const handleFullRefresh = () => {
    setShowIntroAnimation(true);
    showToast('Refreshing POSTO & replaying mail dispatch animation...');
  };

  // Load Messages for Current User Only
  const loadMailbox = async () => {
    if (!currentUser) return;
    setIsLoading(true);
    try {
      let list: Message[] = [];
      if (activeFolder === 'inbox') list = await mailService.getInbox();
      else if (activeFolder === 'sent') list = await mailService.getSent();
      else if (activeFolder === 'starred') list = await mailService.getStarred();
      else if (activeFolder === 'trash') list = await mailService.getTrash();

      // Category Filter
      if (activeCategory === 'unread') {
        list = list.filter(m => !m.is_read);
      } else if (activeCategory === 'starred') {
        list = list.filter(m => m.is_starred);
      } else if (activeCategory === 'attachments') {
        list = list.filter(m => !!m.attachment);
      }

      // Search Filter
      if (searchQuery.trim()) {
        const q = searchQuery.toLowerCase().trim();
        list = list.filter(m => 
          m.subject.toLowerCase().includes(q) ||
          m.body.toLowerCase().includes(q) ||
          m.sender.name.toLowerCase().includes(q) ||
          m.sender.email.toLowerCase().includes(q) ||
          (m.tag && m.tag.toLowerCase().includes(q))
        );
      }

      // Sort Order
      list.sort((a, b) => {
        return sortOrder === 'newest' ? b.created_at - a.created_at : a.created_at - b.created_at;
      });

      setMessages(list);

      // Auto-select first mail on wide screens if none selected
      if (!selectedMail && list.length > 0 && window.innerWidth >= 1024) {
        setSelectedMail(list[0]);
      }

      // Counts
      const [inboxMsgs, starredMsgs, sentMsgs, trashMsgs] = await Promise.all([
        mailService.getInbox(),
        mailService.getStarred(),
        mailService.getSent(),
        mailService.getTrash(),
      ]);

      setCounts({
        inbox: inboxMsgs.filter(m => !m.is_read).length,
        starred: starredMsgs.length,
        sent: sentMsgs.length,
        trash: trashMsgs.length,
      });
    } catch (e) {
      console.error(e);
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    if (currentUser) {
      loadMailbox();
    }
  }, [currentUser, activeFolder, activeCategory, searchQuery, sortOrder]);

  // Auth Submit Handler
  const handleAuthSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setAuthError(null);
    if (!authEmail.trim()) {
      setAuthError('Please enter your email address');
      return;
    }

    try {
      let user: User;
      if (authTab === 'login') {
        user = await mailService.login(authEmail, authPassword);
        showToast(`Welcome back, ${user.name}`);
      } else {
        user = await mailService.register(authName, authEmail, authPassword);
        showToast(`Account registered for ${user.name}`);
      }
      setCurrentUser(user);
      setSelectedMail(null);
    } catch (err: any) {
      setAuthError(err.message || 'Authentication failed');
    }
  };


  const handleLogout = async () => {
    await mailService.logout();
    setCurrentUser(null);
    setSelectedMail(null);
    showToast('Signed out of session');
  };

  // Mail Interaction Handlers
  const handleSelectMail = async (msg: Message) => {
    setSelectedMail(msg);
    if (!msg.is_read) {
      await mailService.markAsRead(msg.id, true);
      setMessages(prev => prev.map(m => m.id === msg.id ? { ...m, is_read: true } : m));
      setCounts(c => ({ ...c, inbox: Math.max(0, c.inbox - 1) }));
    }
  };

  const handleToggleStar = async (e: React.MouseEvent, msgId: number) => {
    e.stopPropagation();
    const updated = await mailService.toggleStar(msgId);
    setMessages(prev => prev.map(m => m.id === msgId ? { ...m, is_starred: updated } : m));
    if (selectedMail && selectedMail.id === msgId) {
      setSelectedMail(prev => prev ? { ...prev, is_starred: updated } : null);
    }
    setCounts(c => ({ ...c, starred: c.starred + (updated ? 1 : -1) }));
  };

  const handleDeleteMail = async (e: React.MouseEvent, msgId: number) => {
    e.stopPropagation();
    if (activeFolder === 'trash') {
      await mailService.permanentDelete(msgId);
      showToast('Permanently deleted mail');
    } else {
      await mailService.deleteMail(msgId);
      showToast('Moved to Trash');
    }
    setMessages(prev => prev.filter(m => m.id !== msgId));
    if (selectedMail && selectedMail.id === msgId) {
      setSelectedMail(null);
    }
    loadMailbox();
  };

  // Trigger Flight Animation when hitting Send Mail
  const handleSendCompose = (e: React.FormEvent) => {
    e.preventDefault();
    if (!composeTo.trim()) {
      showToast('Please specify a recipient');
      return;
    }
    if (!currentUser) return;

    // Launch the cool send flight animation!
    setActiveFlight({
      senderName: currentUser.name,
      senderEmail: currentUser.email,
      recipientEmail: composeTo.trim(),
      subject: composeSubject.trim() || '(No Subject)',
      body: composeBody,
      tag: composeTag,
    });
    setIsComposeOpen(false);
  };

  // Callback when flight animation finishes
  const handleFlightComplete = async () => {
    if (!activeFlight) return;
    try {
      await mailService.sendMail({
        subject: activeFlight.subject,
        body: activeFlight.body,
        recipients: activeFlight.recipientEmail.split(',').map(s => s.trim()).filter(Boolean).map(email => ({ email, type: 'TO' as const })),
        tag: activeFlight.tag,
      });
      showToast(`Mail delivered to ${activeFlight.recipientEmail}! 💌`);
      setComposeTo('');
      setComposeSubject('');
      setComposeBody('');
      setActiveFlight(null);
      loadMailbox();
    } catch (err: any) {
      showToast('Send error: ' + err.message);
      setActiveFlight(null);
    }
  };

  const handleQuickInlineReply = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!selectedMail || !replyText.trim() || !currentUser) return;
    setIsReplying(true);

    // Launch flight animation for quick reply as well!
    setActiveFlight({
      senderName: currentUser.name,
      senderEmail: currentUser.email,
      recipientEmail: selectedMail.sender.email,
      subject: `Re: ${selectedMail.subject}`,
      body: replyText + `\n\n--- In reply to ${selectedMail.sender.name} ---`,
      tag: 'Reply',
    });
    setReplyText('');
    setIsReplying(false);
  };

  const formatDate = (ts: number): string => {
    const d = new Date(ts * 1000);
    return d.toLocaleDateString(undefined, {
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  // ==========================================
  // 1. STARTING ANIMATION
  // ==========================================
  if (showIntroAnimation) {
    return (
      <PostalAnimation
        onComplete={() => setShowIntroAnimation(false)}
      />
    );
  }

  // ==========================================
  // 2. USER LOGIN / REGISTER SCREEN
  // ==========================================
  if (!currentUser) {
    return (
      <div className="min-h-screen bg-[#F6F2EB] text-[#2D1C13] flex flex-col justify-center items-center p-4">
        
        <div className="w-full max-w-md bg-[#FAF5EC] border-2 border-[#D8C4AA] rounded-3xl shadow-xl overflow-hidden flex flex-col animate-in zoom-in-95 duration-200">
          
          <div className="h-2 w-full bg-gradient-to-r from-[#8C5835] via-[#C8733E] to-[#8C5835]" />

          <div className="p-8 flex flex-col items-center">
            
            <Logo size="lg" className="mb-4" />

            <div className="text-[11px] font-mono tracking-widest uppercase font-bold text-[#A86438] bg-[#F2E5D4] px-3.5 py-1 rounded-full border border-[#D8C4AA] mb-4">
              POSTO · RECIPIENT ACCESS
            </div>

            <p className="text-sm font-serif italic text-[#7A5B44] text-center mb-6">
              Enter your credentials to access your private correspondence.
            </p>

            {/* Segmented Sign In / Create Account */}
            <div className="grid grid-cols-2 w-full bg-[#EDE1D0] p-1 rounded-2xl border border-[#D5C1A6] mb-6">
              <button
                type="button"
                onClick={() => { setAuthTab('login'); setAuthError(null); }}
                className={`py-2 text-xs font-bold rounded-xl transition-all ${
                  authTab === 'login'
                    ? 'bg-[#FAF5EC] text-[#2D1C13] shadow-xs'
                    : 'text-[#6D5039] hover:text-[#2D1C13]'
                }`}
              >
                Sign In
              </button>
              <button
                type="button"
                onClick={() => { setAuthTab('signup'); setAuthError(null); }}
                className={`py-2 text-xs font-bold rounded-xl transition-all ${
                  authTab === 'signup'
                    ? 'bg-[#FAF5EC] text-[#2D1C13] shadow-xs'
                    : 'text-[#6D5039] hover:text-[#2D1C13]'
                }`}
              >
                Register
              </button>
            </div>

            {authError && (
              <div className="w-full p-2.5 rounded-xl bg-[#FBEAE8] border border-[#E8BFB7] text-xs text-[#B22222] mb-4 text-center">
                {authError}
              </div>
            )}

            <form onSubmit={handleAuthSubmit} className="w-full flex flex-col gap-3.5">
              {authTab === 'signup' && (
                <div>
                  <label className="text-[11px] font-bold text-[#543825] block mb-1">Full Name</label>
                  <input
                    type="text"
                    required
                    value={authName}
                    onChange={(e) => setAuthName(e.target.value)}
                    placeholder="e.g. Alex Morgan"
                    className="w-full px-3.5 py-2.5 rounded-xl bg-white border border-[#D5C1A6] text-xs text-[#2D1C13] focus:outline-none focus:ring-2 focus:ring-[#C8622A]"
                  />
                </div>
              )}

              <div>
                <label className="text-[11px] font-bold text-[#543825] block mb-1">Email Address</label>
                <input
                  type="email"
                  required
                  value={authEmail}
                  onChange={(e) => setAuthEmail(e.target.value)}
                  placeholder="name@posto.mail"
                  className="w-full px-3.5 py-2.5 rounded-xl bg-white border border-[#D5C1A6] text-xs text-[#2D1C13] focus:outline-none focus:ring-2 focus:ring-[#C8622A]"
                />
              </div>

              <div>
                <label className="text-[11px] font-bold text-[#543825] block mb-1">Password</label>
                <input
                  type="password"
                  required
                  value={authPassword}
                  onChange={(e) => setAuthPassword(e.target.value)}
                  placeholder="••••••••••••"
                  className="w-full px-3.5 py-2.5 rounded-xl bg-white border border-[#D5C1A6] text-xs text-[#2D1C13] focus:outline-none focus:ring-2 focus:ring-[#C8622A]"
                />
              </div>

              <button
                type="submit"
                className="w-full mt-2 py-3 rounded-2xl bg-gradient-to-r from-[#3D2517] to-[#5A3824] hover:from-[#2B180E] hover:to-[#462918] text-[#FAF5EC] text-xs font-bold transition-all shadow-md flex items-center justify-center gap-2"
              >
                <span>{authTab === 'login' ? 'Open Mailbox' : 'Create My Account'}</span>
                <ArrowRight className="w-4 h-4" />
              </button>
            </form>

          </div>

        </div>

      </div>
    );
  }

  // ==========================================
  // 3. MAIN MAILBOX WITH SIDEBAR
  // ==========================================
  return (
    <div className="min-h-screen bg-[#F6F2EB] text-[#2D1C13] flex font-sans selection:bg-[#EBDDC8] selection:text-[#2D1C13] overflow-hidden">
      
      {/* Toast Alert */}
      {toast && (
        <div className="fixed bottom-6 right-6 z-50 bg-[#2D180E] text-[#FAF5EC] px-4 py-2.5 rounded-2xl shadow-xl border border-[#5A3824] text-xs flex items-center gap-2 animate-in fade-in duration-200">
          <Sparkles className="w-4 h-4 text-[#F4A261]" />
          <span>{toast}</span>
        </div>
      )}

      {/* Cool Send Mail Flight Animation */}
      {activeFlight && (
        <SendFlightAnimation
          senderName={activeFlight.senderName}
          senderEmail={activeFlight.senderEmail}
          recipientEmail={activeFlight.recipientEmail}
          subject={activeFlight.subject}
          onComplete={handleFlightComplete}
        />
      )}

      {/* --- CLOSABLE LEFT SIDEBAR --- */}
      {isSidebarOpen && (
        <aside className="w-64 bg-[#FAF6EE] border-r-2 border-[#E2D2BD] shrink-0 flex flex-col justify-between h-screen sticky top-0 z-30 transition-all duration-300">
          
          <div className="p-5 flex flex-col gap-5">
            
            {/* Logo Header + Close Sidebar Button */}
            <div className="flex items-center justify-between">
              <button 
                onClick={() => { setSelectedMail(null); setActiveFolder('inbox'); }}
                className="focus:outline-none"
              >
                <Logo size="md" />
              </button>

              <button
                onClick={() => setIsSidebarOpen(false)}
                className="p-1.5 rounded-lg text-[#8C6D53] hover:text-[#2D1C13] hover:bg-[#EDE1D0] transition-colors"
                title="Close Sidebar"
              >
                <PanelLeftClose className="w-4 h-4" />
              </button>
            </div>

            {/* New Mail CTA */}
            <button
              onClick={() => setIsComposeOpen(true)}
              className="w-full py-2.5 px-4 rounded-xl text-xs font-bold text-white bg-gradient-to-r from-[#3D2619] to-[#5A3824] hover:from-[#2B180E] hover:to-[#462918] transition-all shadow-sm flex items-center justify-center gap-2 border border-[#6B4532]"
            >
              <Plus className="w-4 h-4" />
              <span>New Mail</span>
            </button>

            {/* Mailbox Navigation */}
            <nav className="flex flex-col gap-1">
              <div className="text-[10px] font-mono tracking-wider font-bold text-[#8C6D53] uppercase px-3 py-1">
                Mailbox Folders
              </div>

              {[
                { id: 'inbox', label: 'Inbox', icon: Inbox, count: counts.inbox },
                { id: 'starred', label: 'Starred', icon: Star, count: counts.starred },
                { id: 'sent', label: 'Sent Mail', icon: Send, count: counts.sent }, // Exact label
                { id: 'trash', label: 'Trash', icon: Trash2, count: counts.trash },
              ].map((folder) => {
                const Icon = folder.icon;
                const isActive = activeFolder === folder.id;
                return (
                  <button
                    key={folder.id}
                    onClick={() => {
                      setActiveFolder(folder.id as ActiveFolder);
                      setSelectedMail(null);
                      setIsInboxListOpen(true);
                    }}
                    className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl text-xs font-medium transition-all ${
                      isActive
                        ? 'bg-[#3D2619] text-[#FAF5EC] font-bold shadow-xs'
                        : 'text-[#5E422F] hover:bg-[#EFE3D0] hover:text-[#2D1C13]'
                    }`}
                  >
                    <div className="flex items-center gap-2.5">
                      <Icon className={`w-4 h-4 ${isActive ? 'text-[#F4A261]' : 'text-[#8C6D53]'}`} />
                      <span>{folder.label}</span>
                    </div>
                    {folder.count > 0 && (
                      <span className={`text-[10px] font-mono tabular-nums px-2 py-0.5 rounded-full ${
                        isActive ? 'bg-[#5A3824] text-[#FAF5EC]' : 'bg-[#EADBC8] text-[#6E4931]'
                      }`}>
                        {folder.count}
                      </span>
                    )}
                  </button>
                );
              })}
            </nav>

            {/* Quick Labels / Categories */}
            <div className="flex flex-col gap-1 pt-3 border-t border-[#E8DFC9]">
              <div className="text-[10px] font-mono tracking-wider font-bold text-[#8C6D53] uppercase px-3 py-1">
                Tags & Filters
              </div>

              {[
                { id: 'all', label: 'All Correspondence' },
                { id: 'unread', label: 'Unread Only' },
                { id: 'attachments', label: 'With Attachments' },
              ].map((cat) => (
                <button
                  key={cat.id}
                  onClick={() => {
                    setActiveCategory(cat.id as FilterCategory);
                    setIsInboxListOpen(true);
                  }}
                  className={`w-full text-left px-3 py-1.5 rounded-lg text-xs font-medium transition-colors ${
                    activeCategory === cat.id
                      ? 'bg-[#EDE1D0] font-bold text-[#2D1C13]'
                      : 'text-[#6D5039] hover:bg-[#F2E7D7]'
                  }`}
                >
                  {cat.label}
                </button>
              ))}
            </div>

          </div>

          {/* Bottom Pinned User Profile */}
          <div className="p-4 border-t-2 border-[#E2D2BD] bg-[#F4EDE2] flex items-center justify-between">
            <div className="flex items-center gap-2.5 min-w-0">
              <div className="w-8 h-8 rounded-xl bg-[#3D2619] text-[#FAF5EC] flex items-center justify-center font-bold text-xs shrink-0 shadow-xs">
                {currentUser.name.charAt(0)}
              </div>
              <div className="flex flex-col min-w-0 text-left">
                <span className="text-xs font-bold text-[#2D1C13] truncate">{currentUser.name}</span>
                <span className="text-[10px] font-mono text-[#8C6D53] truncate">{currentUser.email}</span>
              </div>
            </div>

            <button
              onClick={handleLogout}
              className="p-1.5 rounded-lg text-[#8C6D53] hover:text-[#B22222] hover:bg-[#FBEAE8] transition-colors"
              title="Log out"
            >
              <LogOut className="w-4 h-4" />
            </button>
          </div>

        </aside>
      )}

      {/* --- MAIN CONTENT SPLIT PANES --- */}
      <div className="flex-1 flex flex-col min-w-0 h-screen overflow-hidden">
        
        {/* Top Header Search & Controls Bar */}
        <header className="bg-[#FAF6EE] border-b-2 border-[#E2D2BD] px-4 lg:px-6 py-3 flex items-center justify-between gap-3 shrink-0">
          
          <div className="flex items-center gap-3">
            {/* Open Sidebar Button if closed */}
            {!isSidebarOpen && (
              <button
                onClick={() => setIsSidebarOpen(true)}
                className="p-2 rounded-xl bg-white border border-[#D5C1A6] text-[#6D5039] hover:text-[#2D1C13] hover:bg-[#EFE3D0] transition-colors flex items-center gap-1.5 text-xs font-semibold shadow-2xs"
                title="Open Sidebar"
              >
                <PanelLeftOpen className="w-4 h-4 text-[#C8622A]" />
                <span className="hidden sm:inline font-sans">Sidebar</span>
              </button>
            )}

            {/* Toggle Inbox List Button */}
            {selectedMail && (
              <button
                onClick={() => setIsInboxListOpen(!isInboxListOpen)}
                className="p-2 rounded-xl bg-white border border-[#D5C1A6] text-[#6D5039] hover:text-[#2D1C13] hover:bg-[#EFE3D0] transition-colors flex items-center gap-1.5 text-xs font-semibold shadow-2xs"
                title={isInboxListOpen ? 'Close Inbox List' : 'Open Inbox List'}
              >
                {isInboxListOpen ? (
                  <>
                    <EyeOff className="w-4 h-4 text-[#C8622A]" />
                    <span className="hidden sm:inline font-sans">Close Inbox</span>
                  </>
                ) : (
                  <>
                    <Eye className="w-4 h-4 text-[#C8622A]" />
                    <span className="hidden sm:inline font-sans">Open Inbox</span>
                  </>
                )}
              </button>
            )}
          </div>

          {/* Search Box */}
          <div className="relative flex-1 max-w-md">
            <Search className="w-4 h-4 absolute left-3.5 top-1/2 -translate-y-1/2 text-[#8C6D53]" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Search in correspondence..."
              className="w-full pl-10 pr-4 py-2 rounded-xl bg-white border border-[#D8C5AB] text-xs text-[#2D1C13] placeholder-[#A2846D] focus:outline-none focus:ring-1 focus:ring-[#C8622A]"
            />
            {searchQuery && (
              <button
                onClick={() => setSearchQuery('')}
                className="absolute right-3 top-1/2 -translate-y-1/2 text-[#A2846D] hover:text-[#2D1C13]"
              >
                <X className="w-3.5 h-3.5" />
              </button>
            )}
          </div>

          <div className="flex items-center gap-2">
            {/* Sort Toggle */}
            <button
              onClick={() => setSortOrder(o => o === 'newest' ? 'oldest' : 'newest')}
              className="px-3 py-1.5 rounded-xl bg-white border border-[#D5C1A6] text-xs font-medium text-[#4A2D1B] hover:bg-[#EFE3D0] transition-colors flex items-center gap-1.5 shadow-2xs"
            >
              <ArrowUpDown className="w-3.5 h-3.5 text-[#C8622A]" />
              <span className="hidden sm:inline">{sortOrder === 'newest' ? 'Newest' : 'Oldest'}</span>
            </button>

            {/* REFRESH BUTTON (triggers full starting animation replay) */}
            <button
              onClick={handleFullRefresh}
              className="px-3.5 py-1.5 rounded-xl bg-[#EDE1D0] hover:bg-[#E5D5C0] border border-[#D5C1A6] text-xs font-bold text-[#3D2517] transition-all flex items-center gap-1.5 shadow-2xs"
              title="Refresh POSTO and replay the dispatch animation"
            >
              <RefreshCw className="w-3.5 h-3.5 text-[#C8622A]" />
              <span>Refresh</span>
            </button>
          </div>

        </header>

        {/* Master-Detail Split Panes */}
        <div className="flex-1 flex min-h-0 overflow-hidden relative">
          
          {/* CLOSABLE INBOX LIST PANE */}
          {isInboxListOpen && (
            <section className={`${selectedMail ? 'w-full lg:w-[410px]' : 'w-full'} border-r-2 border-[#E2D2BD] bg-[#FAF5EC] flex flex-col overflow-y-auto shrink-0 transition-all duration-300`}>
              
              {/* Header Strip with Close Inbox Button */}
              <div className="px-4 py-2.5 border-b border-[#E8DFC9] bg-[#F4EDE2] flex items-center justify-between text-xs text-[#7A5B44]">
                <div className="flex items-center gap-2">
                  <span className="font-serif font-bold text-sm text-[#2D1C13] capitalize">
                    {activeFolder === 'sent' ? 'Sent Mail' : activeFolder}
                  </span>
                  <span className="font-mono text-[11px] font-semibold">({messages.length})</span>
                </div>

                {selectedMail && (
                  <button
                    onClick={() => setIsInboxListOpen(false)}
                    className="p-1 rounded-md text-[#8C6D53] hover:text-[#2D1C13] hover:bg-[#EAE0D1] text-[11px] font-semibold flex items-center gap-1"
                    title="Close inbox list for full screen view"
                  >
                    <span>Close Inbox</span>
                    <X className="w-3.5 h-3.5" />
                  </button>
                )}
              </div>

              {isLoading ? (
                <div className="p-16 text-center text-xs text-[#8C6D53] flex flex-col items-center justify-center gap-3">
                  <RefreshCw className="w-5 h-5 animate-spin text-[#C8622A]" />
                  <span>Loading mail...</span>
                </div>
              ) : messages.length === 0 ? (
                <div className="p-16 text-center text-xs text-[#8C6D53] flex flex-col items-center justify-center gap-3">
                  <MailOpen className="w-8 h-8 text-[#CBB59D]" />
                  <span className="text-base font-serif font-bold text-[#422616]">
                    No mail found
                  </span>
                  <p className="text-xs text-[#8C6D53]">
                    Try changing category filter or search terms.
                  </p>
                </div>
              ) : (
                <div className="divide-y divide-[#EAE0D0]">
                  {messages.map((msg) => {
                    const isSelected = selectedMail?.id === msg.id;
                    return (
                      <div
                        key={msg.id}
                        onClick={() => handleSelectMail(msg)}
                        className={`p-4 flex flex-col gap-1.5 cursor-pointer transition-colors relative ${
                          isSelected
                            ? 'bg-[#F2E7D7] border-l-4 border-l-[#C8622A]'
                            : !msg.is_read
                            ? 'bg-[#FFFDF9] hover:bg-[#F8EFE2]'
                            : 'bg-[#FAF5EC] hover:bg-[#F3E9DA]'
                        }`}
                      >
                        {/* Row 1: Sender & Time */}
                        <div className="flex items-center justify-between gap-2">
                          <div className="flex items-center gap-2 min-w-0">
                            {!msg.is_read && (
                              <span className="w-2 h-2 rounded-full bg-[#C8622A] shrink-0" />
                            )}
                            <span className={`text-xs truncate ${!msg.is_read ? 'font-bold text-[#2D1C13]' : 'font-medium text-[#4A3222]'}`}>
                              {activeFolder === 'sent' ? `To: ${msg.recipients[0]?.name || msg.recipients[0]?.email}` : msg.sender.name}
                            </span>
                          </div>

                          <div className="flex items-center gap-1.5 shrink-0">
                            <button
                              onClick={(e) => handleToggleStar(e, msg.id)}
                              className="text-[#CBB59D] hover:text-[#C8622A]"
                            >
                              <Star className={`w-3.5 h-3.5 ${msg.is_starred ? 'fill-[#C8622A] text-[#C8622A]' : ''}`} />
                            </button>
                            <span className="text-[10px] font-mono text-[#8C6D53] tabular-nums">
                              {formatDate(msg.created_at).split(',')[0]}
                            </span>
                          </div>
                        </div>

                        {/* Row 2: Subject */}
                        <div className={`text-xs truncate ${!msg.is_read ? 'font-serif font-bold text-[#1C0E07]' : 'font-medium text-[#382317]'}`}>
                          {msg.subject}
                        </div>

                        {/* Row 3: Preview */}
                        <div className="text-[11px] text-[#7A5B44] line-clamp-1 font-normal font-sans">
                          {msg.body.replace(/\n/g, ' ')}
                        </div>

                        {/* Row 4: Tag & Attachment */}
                        <div className="flex items-center gap-1.5 mt-1">
                          {msg.tag && (
                            <span className="text-[9px] font-bold px-1.5 py-0.5 rounded bg-[#EAE0CF] text-[#63442C] border border-[#D5C4AF]">
                              {msg.tag}
                            </span>
                          )}
                          {msg.attachment && (
                            <span className="text-[9px] font-mono flex items-center gap-1 px-1.5 py-0.5 rounded bg-white text-[#7A5B44] border border-[#D5C4AF]">
                              <Paperclip className="w-2.5 h-2.5 text-[#C8622A]" />
                              {msg.attachment.name.slice(0, 14)}...
                            </span>
                          )}
                        </div>
                      </div>
                    );
                  })}
                </div>
              )}

            </section>
          )}

          {/* RIGHT PANE: FULL LETTER READING CANVAS */}
          <main className="flex-1 bg-[#FAF6EE] p-6 lg:p-8 overflow-y-auto flex flex-col">
            {selectedMail ? (
              <div className="max-w-3xl mx-auto w-full flex flex-col gap-6 animate-in fade-in duration-150">
                
                {/* Actions Toolbar */}
                <div className="flex items-center justify-between pb-4 border-b border-[#E8DFC9]">
                  <div className="flex items-center gap-2">
                    {/* Re-open inbox button if inbox list is currently closed */}
                    {!isInboxListOpen && (
                      <button
                        onClick={() => setIsInboxListOpen(true)}
                        className="px-3 py-1.5 rounded-xl bg-white border border-[#D5C1A6] text-xs font-semibold text-[#6D5039] hover:text-[#2D1C13] hover:bg-[#EFE3D0] flex items-center gap-1.5"
                      >
                        <Eye className="w-3.5 h-3.5 text-[#C8622A]" />
                        <span>Open Inbox</span>
                      </button>
                    )}

                    <button
                      onClick={(e) => handleToggleStar(e, selectedMail.id)}
                      className={`px-3 py-1.5 rounded-xl border text-xs font-semibold flex items-center gap-1.5 ${
                        selectedMail.is_starred
                          ? 'bg-[#FBEFDE] border-[#DFC9A8] text-[#C8622A]'
                          : 'bg-white border-[#D5C1A6] text-[#8C6D53]'
                      }`}
                    >
                      <Star className={`w-3.5 h-3.5 ${selectedMail.is_starred ? 'fill-[#C8622A]' : ''}`} />
                      <span>{selectedMail.is_starred ? 'Starred' : 'Star'}</span>
                    </button>

                    {selectedMail.tag && (
                      <span className="text-xs font-bold px-2.5 py-1 rounded-xl bg-[#EDE1D0] text-[#63442C] border border-[#D5C4AF]">
                        {selectedMail.tag}
                      </span>
                    )}
                  </div>

                  <div className="flex items-center gap-2">
                    <button
                      onClick={(e) => handleDeleteMail(e, selectedMail.id)}
                      className="p-2 rounded-xl bg-white border border-[#D5C1A6] text-[#8C6D53] hover:text-[#B22222] hover:bg-[#FBEAE8]"
                      title="Delete"
                    >
                      <Trash2 className="w-4 h-4" />
                    </button>
                  </div>
                </div>

                {/* Subject Header */}
                <h1 className="text-3xl font-serif font-bold text-[#2D1C13] leading-snug">
                  {selectedMail.subject}
                </h1>

                {/* Sender Lockup Card */}
                <div className="p-4 rounded-2xl bg-[#F4EDE2] border border-[#D5C2AA] flex items-center justify-between">
                  <div className="flex items-center gap-3">
                    <div 
                      className="w-11 h-11 rounded-2xl text-[#FAF5EC] flex items-center justify-center font-bold text-sm shadow-xs"
                      style={{ backgroundColor: selectedMail.sender.avatarColor || '#3D2619' }}
                    >
                      {selectedMail.sender.name.charAt(0)}
                    </div>
                    <div>
                      <div className="text-sm font-bold text-[#2D1C13]">{selectedMail.sender.name}</div>
                      <div className="text-xs font-mono text-[#8C6D53]">
                        {selectedMail.sender.email} · To: {selectedMail.recipients.map(r => r.name || r.email).join(', ')}
                      </div>
                    </div>
                  </div>

                  <div className="text-xs font-mono text-[#8C6D53] tabular-nums">
                    {formatDate(selectedMail.created_at)}
                  </div>
                </div>

                {/* Letter Body on Fine Laid Paper */}
                <div className="bg-[#FFFDF9] border border-[#D5C2AA] rounded-2xl p-6 md:p-8 shadow-xs text-sm leading-relaxed text-[#382317] whitespace-pre-wrap font-sans min-h-[220px]">
                  {selectedMail.body}
                </div>

                {/* Attachment Box if Present */}
                {selectedMail.attachment && (
                  <div className="p-4 rounded-2xl bg-[#F4EDE2] border border-[#D5C2AA] flex items-center justify-between">
                    <div className="flex items-center gap-3">
                      <div className="w-9 h-9 rounded-xl bg-white border border-[#D5C2AA] flex items-center justify-center text-[#C8622A]">
                        <Paperclip className="w-4 h-4" />
                      </div>
                      <div>
                        <div className="text-xs font-bold text-[#2D1C13]">{selectedMail.attachment.name}</div>
                        <div className="text-[10px] font-mono text-[#8C6D53]">{selectedMail.attachment.size}</div>
                      </div>
                    </div>

                    <button
                      onClick={() => showToast(`Downloaded ${selectedMail.attachment?.name}`)}
                      className="px-3.5 py-1.5 rounded-xl bg-white border border-[#D5C1A6] text-xs font-semibold text-[#4A2D1B] hover:bg-[#EFE3D0] flex items-center gap-1.5 shadow-2xs"
                    >
                      <Download className="w-3.5 h-3.5 text-[#C8622A]" />
                      <span>Download</span>
                    </button>
                  </div>
                )}

                {/* Quick Inline Reply Box */}
                <form onSubmit={handleQuickInlineReply} className="mt-2 bg-[#F4EDE2] border border-[#D5C2AA] rounded-2xl p-4 flex flex-col gap-3">
                  <div className="flex items-center gap-2 text-xs font-bold text-[#3D2517]">
                    <Reply className="w-4 h-4 text-[#C8622A]" />
                    <span>Quick Reply to {selectedMail.sender.name}</span>
                  </div>

                  <textarea
                    rows={3}
                    required
                    value={replyText}
                    onChange={(e) => setReplyText(e.target.value)}
                    placeholder="Write a response..."
                    className="w-full p-3 rounded-xl bg-white border border-[#D5C1A6] text-xs leading-relaxed text-[#2D1C13] focus:outline-none focus:ring-1 focus:ring-[#C8622A]"
                  />

                  <div className="flex justify-end">
                    <button
                      type="submit"
                      disabled={isReplying || !replyText.trim()}
                      className="px-4 py-2 rounded-xl text-xs font-bold text-white bg-[#3D2619] hover:bg-[#2B180E] disabled:opacity-40 transition-colors flex items-center gap-1.5 shadow-xs"
                    >
                      <Send className="w-3.5 h-3.5" />
                      <span>{isReplying ? 'Sending...' : 'Send Reply'}</span>
                    </button>
                  </div>
                </form>

              </div>
            ) : (
              <div className="m-auto text-center text-xs text-[#8C6D53] flex flex-col items-center justify-center gap-3">
                <MailOpen className="w-10 h-10 text-[#CBB59D]" />
                <span className="text-lg font-serif font-bold text-[#422616]">Select mail to read</span>
                <p className="text-xs text-[#82644D] max-w-xs">
                  Choose any correspondence from your mailbox list to inspect its contents.
                </p>
              </div>
            )}
          </main>

        </div>

      </div>

      {/* --- COMPOSE MODAL --- */}
      {isComposeOpen && (
        <div className="fixed inset-0 z-50 bg-black/45 backdrop-blur-xs flex items-center justify-center p-4">
          <div className="bg-[#FAF5EC] border-2 border-[#D8C4AA] rounded-3xl w-full max-w-2xl shadow-2xl flex flex-col overflow-hidden animate-in zoom-in-95 duration-150">
            
            <div className="px-6 py-4 bg-[#F4EDE2] border-b border-[#E8DFC9] flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Send className="w-4 h-4 text-[#C8622A]" />
                <span className="font-serif font-bold text-base text-[#2D1C13]">Compose New Mail</span>
              </div>
              <button
                onClick={() => setIsComposeOpen(false)}
                className="p-1 rounded-lg text-[#8C6D53] hover:text-[#2D1C13]"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <form onSubmit={handleSendCompose} className="p-6 flex flex-col gap-4">
              <div>
                <label className="text-xs font-bold text-[#543825] block mb-1">To</label>
                <input
                  type="email"
                  required
                  value={composeTo}
                  onChange={(e) => setComposeTo(e.target.value)}
                  placeholder="recipient@example.com"
                  className="w-full px-3.5 py-2.5 rounded-xl bg-white border border-[#D5C1A6] text-xs font-mono text-[#2D1C13] focus:outline-none focus:ring-1 focus:ring-[#C8622A]"
                />
              </div>

              <div>
                <label className="text-xs font-bold text-[#543825] block mb-1">Subject</label>
                <input
                  type="text"
                  required
                  value={composeSubject}
                  onChange={(e) => setComposeSubject(e.target.value)}
                  placeholder="Subject of correspondence..."
                  className="w-full px-3.5 py-2.5 rounded-xl bg-white border border-[#D5C1A6] text-xs font-medium text-[#2D1C13] focus:outline-none focus:ring-1 focus:ring-[#C8622A]"
                />
              </div>

              <div>
                <label className="text-xs font-bold text-[#543825] block mb-1">Tag / Category</label>
                <select
                  value={composeTag}
                  onChange={(e) => setComposeTag(e.target.value)}
                  className="w-full px-3 py-2 rounded-xl bg-white border border-[#D5C1A6] text-xs font-medium text-[#2D1C13]"
                >
                  <option value="General">General</option>
                  <option value="Design">Design</option>
                  <option value="Engineering">Engineering</option>
                  <option value="Urgent">Urgent</option>
                  <option value="Personal">Personal</option>
                </select>
              </div>

              <div>
                <label className="text-xs font-bold text-[#543825] block mb-1">Message</label>
                <textarea
                  rows={6}
                  required
                  value={composeBody}
                  onChange={(e) => setComposeBody(e.target.value)}
                  placeholder="Compose your message..."
                  className="w-full p-4 rounded-xl bg-white border border-[#D5C1A6] text-xs leading-relaxed text-[#2D1C13] focus:outline-none focus:ring-1 focus:ring-[#C8622A]"
                />
              </div>

              <div className="pt-3 border-t border-[#E8DFC9] flex items-center justify-between">
                <span className="text-[11px] text-[#7A5B44]">
                  From: <strong>{currentUser.name}</strong> ({currentUser.email})
                </span>

                <div className="flex items-center gap-2">
                  <button
                    type="button"
                    onClick={() => setIsComposeOpen(false)}
                    className="px-4 py-2 rounded-xl text-xs font-semibold text-[#6D5039]"
                  >
                    Cancel
                  </button>

                  <button
                    type="submit"
                    className="px-5 py-2.5 rounded-xl text-xs font-bold text-white bg-gradient-to-r from-[#3D2619] to-[#5A3824] hover:from-[#2B180E] hover:to-[#462918] transition-all shadow-md flex items-center gap-2"
                  >
                    <Send className="w-3.5 h-3.5" />
                    <span>Send Mail</span>
                  </button>
                </div>
              </div>
            </form>

          </div>
        </div>
      )}

    </div>
  );
}
