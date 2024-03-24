<script lang="ts">
    import type { Attendance, Course } from '$lib/models';
    import { attendanceStore, courseStore } from '$lib/store';
    import { mockAttendance, mockCourses } from '$lib/mock-data';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';

    let course: Course | undefined = undefined;
    onMount(() => {
        attendanceStore.set(mockAttendance);
        course = $courseStore.find(
            (c) => c.id.toString() == $page.params.courseId
        );
    });
</script>

<div class="p-8">
    <h2 class="h2">Attendance for {course ? course.course_code : $page.params.courseId}</h2>
    <div class="grid grid-cols-1 gap-4 mt-8">
        {#each $attendanceStore as attendance}
            {#if attendance.course_id === $page.params.courseId}
                <a
                    href="/teacherApp/attendance/{$page.params.courseId}/{attendance.date}"
                    class="card variant-soft-surface p-4"
                >
                    <h3>{attendance.date}</h3>
                </a>
            {/if}
        {/each}
        <a href="/teacherApp/attendance/{$page.params.courseId}/add" class="btn variant-filled-primary">
            <i class="fi fi-tr-square-plus"></i>
            Add Attendance
        </a>
    </div>
</div>