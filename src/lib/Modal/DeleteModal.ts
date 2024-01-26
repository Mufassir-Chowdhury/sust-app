import Swal from 'sweetalert2';
import { deleteItemFromDatabase } from "$lib/Database/delete";
import { getID } from '$lib/utils';

export function onDelete(id: string) {
    Swal.fire({
        title: "Are you sure?",
        text: "You won't be able to revert this!",
        icon: "warning",
        showCancelButton: true,
        confirmButtonColor: "#3085d6",
        cancelButtonColor: "#d33",
        confirmButtonText: "Yes, delete it!"
        }).then(async (result) => {
        if (result.isConfirmed) {
            await deleteItemFromDatabase(id);
            Swal.fire({
                title: "Deleted!",
                text: `${getID(id)} has been deleted.`,
                icon: "success"
            }).then(() => {
                location.reload();
            });
        }
    });
}