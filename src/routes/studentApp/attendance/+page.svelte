<script lang="ts">
    import { onMount } from "svelte";

    export let data;
</script>

<div class="p-8">
    <h2 class="h2">Attendance</h2>
    <div class="grid grid-cols-1 gap-4 mt-8">
        {#each data.coursesWithAttendance as course}
            <a href="/studentApp/attendance/{course.id}" class="card variant-soft-surface p-4">
                <h3>{course.name}</h3>
                <p>Semester: {course.semester}</p>
                <p>Present: {course.presentCount} | Absent: {course.absentCount}</p>
            </a>
        {/each}
    </div>
</div>