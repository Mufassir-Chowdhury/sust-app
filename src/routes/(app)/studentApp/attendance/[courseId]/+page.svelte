<script lang="ts">
    import { onMount } from "svelte";
    import { page } from "$app/stores";

    export let data;

</script>

<div class="p-8">
    <h2 class="h2">Attendance for {data.course ? data.course.course_code : $page.params.courseId}</h2>
    <div class="grid grid-cols-1 gap-4 mt-8">
        <table class="table-auto w-full">
            <thead>
                <tr>
                    <th class="px-4 py-2">Date</th>
                    <th class="px-4 py-2">Attendance</th>
                </tr>
            </thead>
            <tbody>
                {#each data.attendances as attendance}
                    <tr>
                        <td class="border px-4 py-2 text-center">{attendance.date}</td>
                        <td class="border px-4 py-2 text-center">
                            {#if attendance.attendance[data.user] === true}
                                <span class="text-green-500">Present</span>
                            {:else}
                                <span class="text-red-500">Absent</span>
                            {/if}
                        </td>
                    </tr>
                {/each}
            </tbody>
        </table>
    </div>
</div>