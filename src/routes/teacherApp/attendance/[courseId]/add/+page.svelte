<script lang="ts">
    import { attendanceStore, studentStore, courseStore } from '$lib/store';
    import { mockStudents } from '$lib/mock-data';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';
    import type { Student, Attendance, Course } from '$lib/models';

    let students: Student[] = [];
    let attendanceMap: { [key: string]: boolean } = {};
    let course: Course | undefined = undefined;

    onMount(() => {
        studentStore.set(mockStudents);
        students = $studentStore;
        course = $courseStore.find(
            (c) => c.id.toString() == $page.params.courseId
        );
    });

    function toggleAttendance(studentId: string) {
        attendanceMap[studentId] = !attendanceMap[studentId];
    }

    async function submitAttendance() {
        const newAttendance = {
            course_id: $page.params.courseId,
            date: new Date().toISOString().slice(0, 10),
            students: attendanceMap,
        };
        attendanceStore.update((attendances) => [...attendances, newAttendance]);
        // Reset the attendance map
        attendanceMap = {};
    }
</script>

<div class="p-8">
    <h2 class="h2">Attendance for {course ? course.course_code : $page.params.courseId}</h2>
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
                    {#each students as student}
                    <tr>
                        <td class="border px-4 py-2 text-center">{student.id}</td>
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
