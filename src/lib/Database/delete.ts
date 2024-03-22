import { connectToDb, database, db } from "./surreal";

export async function deleteItemFromDatabase(id: string){
    if(!database) return;
    await connectToDb();
    await db.delete(id); 
}