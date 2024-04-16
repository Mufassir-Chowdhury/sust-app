<script lang="ts">
    import { goto } from '$app/navigation';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';
	import { getID } from '$lib/utils.js';

    export let data;
    let attendanceMap: { [key: string]: boolean } = {};

    onMount(() => {
        data.students.forEach(student => {
            attendanceMap[student.id] = false;
        });
    });

    function toggleAttendance(studentId: string) {
        attendanceMap[studentId] = !attendanceMap[studentId];
    }

    async function submitAttendance() {
        const newAttendance = {
            id: Math.random().toString(36).substr(2, 9),
            course_id: $page.params.courseId,
            date: new Date().toISOString().slice(0, 10),
            students: attendanceMap,
        };

        const formData = new FormData();
        formData.append('attendance', JSON.stringify(newAttendance));

        try {
            const response = await fetch(`?/formAction`, {
                method: 'POST',
                body: formData,
            });

            if (response.ok) {
                attendanceMap = {};
                goto(`/teacherApp/attendance/${$page.params.courseId}`);
            } else {
                console.error('Error submitting attendance:', response.status);
            }
        } catch (error) {
            console.error('Error submitting attendance:', error);
        }
    }
</script>

<div class="p-8">
    <h2 class="h2">Attendance for {data.course ? data.course.course_code : $page.params.courseId}</h2>
        <div class="grid grid-cols-1 gap-4 mt-8">
            <table class="table-auto w-full">
                <thead>
                    <tr>
                        <th class="px-4 py-2">Student ID</th>
                        <th class="px-4 py-2">Name</th>
                        <th class="px-4 py-2">Attendance</th>
                    </tr>
                </thead>
                <tbody>
                    {#each data.students as student}
                    <tr>
                        <td class="border px-4 py-2 text-center">{getID(student.id)}</td>
                        <td class="border px-4 py-2 text-center">{student.name}</td>
                        <td class="border px-4 py-2 text-center">
                            <input
                                type="checkbox"
                                class="form-checkbox h-5 w-5 text-green-500"
                                on:change={() => toggleAttendance(student.id)}
                                checked={attendanceMap[student.id]}
                            />
                        </td>
                    </tr>
                    {/each}
                </tbody>
            </table>
            <button class="btn variant-filled-primary" on:click={submitAttendance}
                >Submit Attendance</button
            >
        </div>
    </div>
