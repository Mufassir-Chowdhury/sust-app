<script lang="ts">
    import { page } from '$app/stores';

    export let data;
</script>

<div class="p-8">
    <h2 class="h2">Attendance for {data.course ? data.course.course_code : $page.params.courseId}</h2>
    <div class="grid grid-cols-1 gap-4 mt-8">
        {#each data.attendances as attendance}
            <a
                href="/teacherApp/attendance/{$page.params.courseId}/{attendance.id}"
                class="card variant-soft-surface p-4"
            >
                <h3>{attendance.date}</h3>
            </a>
        {/each}
        <a href="/teacherApp/attendance/{$page.params.courseId}/add" class="btn variant-filled-primary">
            <i class="fi fi-tr-square-plus"></i>
            Add Attendance
        </a>
    </div>
</div>