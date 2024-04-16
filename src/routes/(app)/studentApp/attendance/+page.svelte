<script lang="ts">

    export let data;
</script>

<div class="p-8">
    <h2 class="h2">Attendance</h2>
    <div class="grid grid-cols-1 gap-4 mt-8">
        {#each data.courses as course}
            <a href="/studentApp/attendance/{course.id}" class="card variant-soft-surface p-4">
                <h3>{course.name}</h3>
                <p>{course.id}</p>
            </a>
        {/each}
    </div>
</div>