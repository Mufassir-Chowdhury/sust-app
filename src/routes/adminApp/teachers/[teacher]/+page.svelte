<script lang="ts">
	import Detailsitem from "$lib/Components/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<h3 class="title">{details.name}</h3>
<Detailsitem key="ID" value={getID(details.id)}/>
<Detailsitem key="Department" value={data.department}/>
<Detailsitem key="Designation" value={details.designation}/>
<Detailsitem key="Email" value={details.email.academic}/>
<Detailsitem key="Gender" value={details.gender}/>
<Detailsitem key="Blood Group" value={details.blood_group}/>
