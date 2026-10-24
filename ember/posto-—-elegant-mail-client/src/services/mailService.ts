import type { ApiMessage, ApiRecipient } from '../types/api';
import { request } from './httpClient';

export const mailService = {
  list: () => request<ApiMessage[]>('/messages'),
  detail: (id: number) => request<ApiMessage>(`/messages/${id}`),
  send: (payload: { subject: string; body: string; recipients: ApiRecipient[] }) =>
    request<{ id: number }>('/messages', { method: 'POST', body: JSON.stringify(payload) }),
  remove: (id: number) => request<{ status: string }>(`/messages/${id}`, { method: 'DELETE' }),
};
