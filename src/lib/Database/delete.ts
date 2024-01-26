import { connectToDb, db } from "./surreal";

export async function deleteItemFromDatabase(id: string){
    console.log('deleting ' + id);
    await connectToDb();
    await db.delete(id); 
}