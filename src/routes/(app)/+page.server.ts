import type { PageServerLoad } from './$types';
import { type Actions, redirect } from '@sveltejs/kit';
import { db } from '$lib/Database/surreal';

export const load = (async ({ locals }) => {
	return { locals };
}) satisfies PageServerLoad;

export const actions = {
	logout: async (event) => {
		event.locals.user = undefined;
		event.cookies.set('token', '', {
			path: '/',
			httpOnly: true,
			sameSite: 'strict',
			secure: process.env.NODE_ENV === 'production',
			maxAge: -1
		});

		await db.invalidate();
		throw redirect(303, '/signin');
	}
} satisfies Actions;
