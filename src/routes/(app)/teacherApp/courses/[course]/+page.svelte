<script lang="ts">
	import Detailsitem from "$lib/Components/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
    
</script>
<h3 class="title">{details.title ?? details.name}</h3>
<div class="w-full grid grid-cols-5">
    <div class="col-span-3">

        <Detailsitem key="Name" value={details.title ?? details.name}/>
        <Detailsitem key="ID" value={getID(details.id)}/>
        <Detailsitem key="Credit" value={details.credit}/>
        <Detailsitem key="Department" value={data.department}/>
        <Detailsitem key="Type" value={details.type}/>
        <Detailsitem key="Year" value={details.year}/>
        <Detailsitem key="Semester" value={details.semester}/>
        <div class="flex justify-between m-4">
            <h3 class="h3">Instructors:</h3>
        </div>
        {#if data.instructors.length == 0}
            <p>No instructors assigned</p>
        {/if}
        {#each data.instructors as instructor}
            <Detailsitem key={instructor.id} value={instructor.name}/>
        {/each}
        <div class="flex justify-between m-4">
            <h3 class="h3">Students:</h3>
        </div>
        {#if data.students.length == 0}
            <p>No students took this course yet</p>
        {/if}
        {#each data.students as student}
            <Detailsitem key={student.id} value={student.name}/>
        {/each}
    </div>
    <div class="col-span-2">
        <div class="flex justify-between m-4">
            <h3 class="h3">Assignments:</h3>
        </div>
        {#if data.assignments.length == 0}
            <p>No assignments for this course yet</p>
        {/if}
        <div class="flex flex-col mx-4 gap-4">
            {#each data.assignments as assignment}
                <a href="/teacherApp/assignments/{details.id}/{assignment.id}" class="w-full card variant-soft-surface p-4">
                    <h3>{assignment.title}</h3>
                    <p>{assignment.id}</p>
                </a>
            {/each}
        </div>
        <div class="flex justify-between m-4">
            <h3 class="h3">Exams:</h3>
        </div>
        {#if data.exams.length == 0}
            <p>No exams for this course yet</p>
        {/if}
        <div class="flex flex-col mx-4 gap-4">
            {#each data.exams as exam}
                <a href="/teacherApp/exams/{details.id}/{exam.id}" class="w-full card variant-soft-surface p-4">
                    <h3>{exam.title}</h3>
                    <p>{exam.id}</p>
                </a>
            {/each}
        </div>
        <div class="flex justify-between m-4">
            <h3 class="h3">Classes:</h3>
        </div>
        {#if data.attendances.length == 0}
            <p>No classes takes yet</p>
        {/if}
        <div class="flex flex-col mx-4 gap-4">
            {#each data.attendances as attendance}
                <a href="/teacherApp/attendance/{details.id}/{attendance.id}" class="w-full card variant-soft-surface p-4">
                    <h3>{attendance.date}</h3>
                    <p>{attendance.id}</p>
                </a>
            {/each}
        </div>
    </div>
</div>