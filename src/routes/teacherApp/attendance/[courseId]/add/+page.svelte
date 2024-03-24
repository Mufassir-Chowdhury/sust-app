<script lang="ts">
    import { goto } from '$app/navigation';
    import { attendanceStore, studentStore, courseStore, courseEnrollmentStore } from '$lib/store';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';
    import type { Student, Attendance, Course } from '$lib/models';

    let studentIds: string[] | undefined = undefined;
    let students: Student[] = [];
    let attendanceMap: { [key: string]: boolean } = {};
    let course: Course | undefined = undefined;

    onMount(() => {
        studentIds = $courseEnrollmentStore.find(
            (c) => c.course_id.toString() == $page.params.courseId
        )?.studentsIds;
        students = $studentStore.filter((s) => studentIds?.includes(s.id));
        course = $courseStore.find(
            (c) => c.id.toString() == $page.params.courseId
        );
        students.forEach(student => {
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
        attendanceStore.update((attendances) => [...attendances, newAttendance]);
        // Reset the attendance map
        attendanceMap = {};
        goto(`/teacherApp/attendance/${$page.params.courseId}`);
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
