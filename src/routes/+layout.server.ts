import { connectToDb } from '$lib/Database/surreal';

export async function load() {
    try {
        await connectToDb();
    } catch (e) {
        console.error('ERROR', e);
    }
}