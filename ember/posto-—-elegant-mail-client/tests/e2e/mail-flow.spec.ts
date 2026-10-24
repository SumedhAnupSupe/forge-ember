import { test, expect, type Page } from '@playwright/test';

async function register(page: Page, name: string, email: string, password: string) {
  await page.goto('/');
  await page.getByRole('button', { name: 'Register' }).click();
  await page.getByPlaceholder('e.g. Alex Morgan').fill(name);
  await page.getByPlaceholder('name@posto.mail').fill(email);
  await page.locator('input[type="password"]').fill(password);
  await page.getByRole('button', { name: 'Create My Account' }).click();
  await expect(page.getByRole('button', { name: 'New Mail' })).toBeVisible({ timeout: 10000 });
}

test('two registered users can send, receive, and find persisted mail', async ({ browser }) => {
  const suffix = `${Date.now()}-${Math.floor(Math.random() * 100000)}`;
  const senderEmail = `sender-${suffix}@posto.test`;
  const recipientEmail = `recipient-${suffix}@posto.test`;
  const password = 'E2e-Test-Password-42!';
  const subject = `E2E correspondence ${suffix}`;
  const body = 'End-to-end delivery verification from POSTO through EMBER into FORGE.';

  const senderContext = await browser.newContext();
  const sender = await senderContext.newPage();
  const recipientContext = await browser.newContext();
  const recipient = await recipientContext.newPage();

  try {
    await register(sender, 'E2E Sender', senderEmail, password);
    await register(recipient, 'E2E Recipient', recipientEmail, password);

    await sender.getByRole('button', { name: 'New Mail' }).click();
    await sender.getByPlaceholder('recipient@example.com').fill(recipientEmail);
    await sender.getByPlaceholder('Subject of correspondence...').fill(subject);
    await sender.getByPlaceholder('Compose your message...').fill(body);
    await sender.getByRole('button', { name: 'Send Mail' }).click();

    await expect(sender.getByText('Mail Successfully Delivered!', { exact: true })).toBeVisible({ timeout: 7000 });
    await expect(sender.getByText(new RegExp(`Mail delivered to ${recipientEmail.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}`))).toBeVisible({ timeout: 10000 });

    await sender.getByRole('button', { name: 'Sent Mail' }).click();
    await expect(sender.getByText(subject, { exact: true })).toBeVisible({ timeout: 10000 });

    await recipient.getByRole('button', { name: 'Refresh' }).click().catch(() => {});
    await expect(recipient.getByText('Inbox', { exact: true })).toBeVisible({ timeout: 10000 });
    await expect(recipient.getByText(subject, { exact: true })).toBeVisible({ timeout: 10000 });
    await recipient.getByText(subject, { exact: true }).click();
    await expect(recipient.getByText(body, { exact: true })).toBeVisible({ timeout: 10000 });
  } finally {
    await senderContext.close();
    await recipientContext.close();
  }
});

test('invalid login shows an error and does not open the mailbox', async ({ page }) => {
  await page.goto('/');
  await page.getByPlaceholder('name@posto.mail').fill(`missing-${Date.now()}@posto.test`);
  await page.locator('input[type="password"]').fill('Definitely-Wrong-Password-42!');
  await page.getByRole('button', { name: 'Open Mailbox' }).click();
  await expect(page.getByText('Invalid credentials')).toBeVisible({ timeout: 10000 });
  await expect(page.getByRole('button', { name: 'New Mail' })).toHaveCount(0);
});
