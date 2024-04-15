import { error, redirect, type Handle } from '@sveltejs/kit';
import { sequence } from '@sveltejs/kit/hooks';
import { db } from '$lib/Database/surreal';
import { logout } from '$lib/Database/logout';


// Authentication and Authorization
const auth = (async ({ event, resolve }) => {
	const { cookies } = event;
	const token = cookies.get('token'); // Get token from cookie.
	const secureRoute = event.route.id?.includes('(app)'); // Detect protected route.
	const loginRoute = event.url.pathname.includes('/signin'); // Detect login form.
	const authRoute = loginRoute;
	// If cookie with token exist and user is under protected route, authenticate user in SurrealDB.
	if (token && secureRoute) {
		const authenticated = await db.authenticate(token).catch(async (err: Error) => {
			console.log(`Error: ${err.message}. Session invalidation.`);
			// If something wrong with token - invalidate session client side, server side and in SurrealDB.
			await logout(event);
		});
		if (authenticated) {
			// Get authenticated user info and add it to request
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
			// If not authenticated - invalidate session client side, server side and in SurrealDB.
			await logout(event);
		}
	}

	if (secureRoute) {

		if (!event.locals.user || !token) {

			// If user is in protected route, but cookie with token or user info in request is missing - invalidate session client side, server side and in SurrealDB.
			await logout(event);
		}
	}
	if (authRoute && token) {
		// Prevent logged in users from navigating to login / register forms.
		redirect(303, '/');
	}
	// console.log(event.locals.user);
	// If no problems found - let's go.
	const response = await resolve(event);
	return response;
}) satisfies Handle;

export const handle = sequence(auth);
