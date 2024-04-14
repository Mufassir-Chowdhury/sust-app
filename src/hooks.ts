import { connectToDb, db } from '$lib/Database/surreal';

export async function load() {
    try {
        await connectToDb();
    } catch (e) {
        console.error('ERROR', e);
    }
}

import { setContext } from 'svelte';
export async function authenticate(email: string, password: string, scope: string) {
  try {
    const session = await db.signin({
        namespace: 'test',
        database: 'test',
        scope: scope,

        // Also pass any properties required by the scope definition
        email: email,
        pass: password,
    });

    setContext('session', session);
    return session;
  } catch (error) {
    console.error('Authentication error:', error);
    throw error;
  }
}