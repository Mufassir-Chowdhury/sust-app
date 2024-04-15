import { error, redirect, type Handle } from '@sveltejs/kit';
import { sequence } from '@sveltejs/kit/hooks';
import { db } from '$lib/Database/surreal';
import { logout } from '$lib/Database/logout';


const auth = (async ({ event, resolve }) => {
	const { cookies } = event;
	const token = cookies.get('token'); 
	const secureRoute = event.route.id?.includes('(app)');
	const loginRoute = event.url.pathname.includes('/signin'); 
	const authRoute = loginRoute;
	if (token && secureRoute) {
		const authenticated = await db.authenticate(token).catch(async (err: Error) => {
			console.log(`Error: ${err.message}. Session invalidation.`);
			await logout(event);
		});
		if (authenticated) {
			if (!event.locals.user) {
				const user = (await db.info().catch((err: Error) => {
					console.log(`error: ${err.message}`);
					error(500, 'Something wrong with database connection.');
				})) as any;
				const department = await db.select(user.department);
				user.departmentName = department[0].name
				event.locals.user = user;
			}
		} else {
			await logout(event);
		}
	}

	if (secureRoute) {
		if (!event.locals.user || !token) {
			await logout(event);
		}
	}
	if (authRoute && token) {
		redirect(303, '/');
	}
	const response = await resolve(event);
	return response;
}) satisfies Handle;

export const handle = sequence(auth);
