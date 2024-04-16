import { Surreal } from 'surrealdb.js';
import { PUBLIC_SURREALDB_URL } from '$env/static/public';
import type {Writable } from 'svelte/store';

const MAX_RETRIES = 5;
const RETRY_TIMEOUT = 2000; // 2 seconds
const DB_URL = PUBLIC_SURREALDB_URL;
let _db: Surreal;


const databaseInstance = {
	get instance() {
		if (!_db) {
			let retries = 1;

			const tryConnect = async () => {
				try {
					if (retries > 1) {
						console.log(`Database connection retry, attempt number ${retries} of ${MAX_RETRIES}`);
					}
					_db = new Surreal();

					if (!DB_URL) return null;
					await _db.connect(DB_URL, { namespace: 'test', database: 'test'
					, auth: {
                                        namespace: 'test',
                                        database: 'test',
                                        username: 'root',
                                        password: 'root',
                                    },
								 });
				} catch (error) {
					if (retries < MAX_RETRIES) {
						retries++;
						setTimeout(tryConnect, RETRY_TIMEOUT);
					} else {
						console.log('Database connection failed.');
						throw error;
					}
				}
			};

			tryConnect();
		}
		return _db;
	},
};

export const db = databaseInstance.instance;

export const observeLive = async <T extends Record<string, unknown>>(thing: string, store: Writable<T[]>) => {
		await db.live(thing, ({ action, result }) => {

			// TODO: on 'CREATE' and 'UPDATE' author (user) object is not fetched, this results in undefined in author.username
			switch (action) {
				case 'CREATE':
					return store.update((data) => [ result as T, ...data]);
				case 'UPDATE':
					return store.update((data) =>
						data.map((record) => (record.id === result.id ? result as T : record))
					);
				case 'DELETE':
					return store.update((data) => data.filter((record) => record.id !== result));
				case 'CLOSE':
					return;
			}
		});
};

export const database = true;

// export const db = new Surreal();
// export async function connectToDb() {
//     if(!database) return;
//     try {
//         // Connect to the database
//         await db.connect('http://127.0.0.1:8000/rpc', {
//             // Set the namespace and database for the connection
//             namespace: 'test',
//             database: 'test',

//             // Set the authentication details for the connection
//             auth: {
//                 namespace: 'test',
//                 database: 'test',
//                 username: 'root',
//                 password: 'root',
//             },
//         });
//     } catch (e) {
//         console.error('ERROR', e);
//     }
// }