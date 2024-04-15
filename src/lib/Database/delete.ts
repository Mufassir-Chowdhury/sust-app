import { database, db } from "./surreal";

export async function deleteItemFromDatabase(id: string){
    if(!database) return;
    await db.delete(id); 
}