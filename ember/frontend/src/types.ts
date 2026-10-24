export interface User { id: number; name: string; email: string; created_at?: number; }
export interface Recipient { id?: number; name?: string; email: string; type: 'TO' | 'CC' | 'BCC'; }
export interface Message {
  id: number; sender_id?: number; sender?: { id: number; name: string; email: string };
  recipients?: Recipient[]; subject: string; body: string; created_at: number;
  is_read?: boolean | number; is_starred?: boolean | number; is_deleted?: boolean | number;
}
