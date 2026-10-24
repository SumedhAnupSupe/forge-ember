/**
 * POSTO Type Contracts
 */

export interface User {
  id: number;
  name: string;
  email: string;
  created_at: number;
}

export type RecipientType = 'TO' | 'CC' | 'BCC';

export interface Recipient {
  id: number;
  name: string;
  email: string;
  type: RecipientType;
}

export interface Message {
  id: number;
  sender: {
    id: number;
    name: string;
    email: string;
    avatarColor?: string;
  };
  recipients: Recipient[];
  subject: string;
  body: string;
  created_at: number;
  is_read: boolean;
  is_starred: boolean;
  is_deleted: boolean;
  tag?: string;
  attachment?: {
    name: string;
    size: string;
  };
  wax_seal?: 'monogram' | 'oak' | 'crest' | 'star';
}

export interface ApiError {
  success: false;
  error: {
    code: string;
    message: string;
  };
}
