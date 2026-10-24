export interface ApiUser { id: number; name: string; email: string; created_at?: number; }
export interface ApiRecipient { user_id: number; type: 'TO' | 'CC' | 'BCC'; }
export interface ApiMessage { id: number; sender_id: number; subject: string; body: string; created_at: number; }
export interface ApiErrorPayload { success: false; error: { code: string; message: string }; }
