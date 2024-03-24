<script lang="ts">
    import { attendanceStore, studentStore, courseStore } from '$lib/store';
    import { mockStudents } from '$lib/mock-data';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';
    import type { Student, Attendance, Course } from '$lib/models';

    let attendance: Attendance | undefined = undefined;
    let students: Student[] = [];
    let course: Course | undefined = undefined;

    onMount(() => {
        attendance = $attendanceStore.find(
            (a) =>
                a.id == $page.params.attendanceId
        );
        course = $courseStore.find(
            (c) => c.id == $page.params.courseId
        );
        if (attendance) {
            students = $studentStore.filter(
                (s) => attendance && attendance.students[s.id] !== undefined
            );
        }
    });
</script>

<div class="p-8">
    <h2 class="h2">Attendance for {course ? course.course_code : $page.params.courseId} on {attendance?.date}</h2>
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
                            {#if attendance?.students[student.id]}
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