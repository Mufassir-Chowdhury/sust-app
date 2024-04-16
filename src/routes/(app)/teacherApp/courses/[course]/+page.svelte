<script lang="ts">
	import Detailsitem from "$lib/Components/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
    
</script>
<h3 class="title">{details.title ?? details.name}</h3>
<Detailsitem key="Name" value={details.title ?? details.name}/>
<Detailsitem key="ID" value={getID(details.id)}/>
<Detailsitem key="Credit" value={details.credit}/>
<Detailsitem key="Department" value={data.department}/>
<Detailsitem key="Type" value={details.type}/>
<Detailsitem key="Year" value={details.year}/>
<Detailsitem key="Semester" value={details.semester}/>
<div class="flex justify-between m-4">
    <h2>Instructors:</h2>
</div>
{#if data.instructors.length == 0}
    <p>No instructors assigned</p>
{/if}
{#each data.instructors as instructor}
    <a href="/adminApp/teachers/{instructor.id}">
        <Detailsitem key={instructor.id} value={instructor.name}/>
    </a>
{/each}