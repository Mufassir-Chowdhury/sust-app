import { redirect, type RequestEvent } from '@sveltejs/kit';
import { db } from './surreal';

export const logout = async (event: RequestEvent, redirect_to?: string) => {
	event.locals.user = undefined;
	event.cookies.set('token', '', {
		path: '/',
		httpOnly: true,
		sameSite: 'strict',
		secure: process.env.NODE_ENV === 'production',
		maxAge: -1
	});
	await db.invalidate();
	if (redirect_to) {
		throw redirect(302, redirect_to);
	} else {
		throw redirect(302, '/signin');
	}
};
