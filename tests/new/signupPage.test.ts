import { expect, test } from '@playwright/test';

test.describe('Sign Up Page', () => {
	test.beforeEach(async ({ page }) => {
		await page.goto('/signin');
	});

	test('should display the sign up form', async ({ page }) => {
		const form = await page.$('form');
		expect(form).toBeTruthy();
	});

	test('should not allow invalid email', async ({ page }) => {
		await page.fill('input[type="text"]', 'invalidemail');
		await page.fill('input[type="password"]', 'password123');
		await page.click('button');
		const error = await page.$('.error');
		expect(error).toBeTruthy();
	});

	test('should not allow invalid password', async ({ page }) => {
		await page.fill('input[type="text"]', 'test@example.com');
		await page.fill('input[type="password"]', 'pass');
		await page.click('button');
		const error = await page.$('.error');
		expect(error).toBeTruthy();
	});

	test('should allow valid email and password', async ({ page }) => {
		await page.fill('input[type="text"]', 'test@example.com');
		await page.fill('input[type="password"]', 'password123');
		await page.click('button');
		const error = await page.$('.error');
		expect(error).toBeFalsy();
	});
});