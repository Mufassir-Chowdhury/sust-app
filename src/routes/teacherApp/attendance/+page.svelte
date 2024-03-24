<script lang="ts">
    import { courseStore } from '$lib/store';
    import { mockCourses } from '$lib/mock-data';
    import { onMount } from 'svelte';

    onMount(() => {
    });
</script>

<div class="p-8">
    <h2 class="h2">Attendance</h2>
    <div class="grid grid-cols-3 gap-4 mt-8">
        {#each $courseStore as course}
            <a href="/teacherApp/attendance/{course.id}" class="card variant-soft-surface p-4">
                <h3>{course.name}</h3>
                <p>{course.course_code}</p>
            </a>
        {/each}
    </div>
</div>