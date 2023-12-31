import { Surreal } from 'surrealdb.js';

export const db = new Surreal();

export async function connectToDb() {
    try {
        // Connect to the database
        await db.connect('http://127.0.0.1:8000/rpc', {
            // Set the namespace and database for the connection
            namespace: 'test',
            database: 'test',

            // Set the authentication details for the connection
            auth: {
                namespace: 'test',
                database: 'test',
                username: 'root',
                password: 'root',
            },
        });
    } catch (e) {
        console.error('ERROR', e);
    }
}