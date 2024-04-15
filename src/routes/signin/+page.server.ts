import { fail, type Actions, redirect, error } from '@sveltejs/kit';
import { db } from '$lib/Database/surreal';

export const actions = {
	login: async ({ request, locals, cookies }) => {
		if (locals.user) redirect(303, '/');
		const form = await request.formData();
		const email = form.get('email');
		const password = form.get('password');
		const scope = form.get('scope') as string;
		await db.use({ namespace: 'test', database: 'test' }).catch((err: Error) => {
			error(500, `Error: ${err.message}`);
		});

		try {
			await db
				.signin({
					scope: scope,
					email: email,
					password: password
				})
				.then(async (token) => {
					if (!token) error(401, 'Authentication failed');
					cookies.set('token', token, {
						path: '/',
						httpOnly: true,
						sameSite: 'strict',
						secure: process.env.NODE_ENV === 'production',
						maxAge: 60 * 60 * 24 * 7, // 1 week
						priority: 'high'
					});
					console.log(token);
				});
		} catch (err) {
			return fail(400, { email: email, password: password, scope: scope });
		}
		if(scope === 'admin') {
			throw redirect(303, '/adminApp');
		} else if(scope === 'teacher') {
			throw redirect(303, '/teacherApp');
		} else if(scope === 'student') {
			throw redirect(303, '/studentApp');
		}
	}
} satisfies Actions;
