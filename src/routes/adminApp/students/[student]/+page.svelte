<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        title={details.name}>
    <Detailsitem key="ID" value={getID(details.id)}/>
    <Detailsitem key="Department" value={data.department}/>
    <Detailsitem key="Email" value={details.email.academic}/>
    <Detailsitem key="Gender" value={details.gender}/>
    <Detailsitem key="Blood Group" value={details.blood_group}/>
    <Detailsitem key="Current Semester" value={details.current_semester}/>
    <Detailsitem key="Session" value={details.session}/>
</DetailsPage>