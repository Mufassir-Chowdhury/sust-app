<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        url={data.url}
        appUrl={data.appUrl}
        item={details.name}
        title={details.name}>
    <Detailsitem key="ID" value={getID(details.id)}/>
    <Detailsitem key="Department" value={data.department}/>
    <Detailsitem key="Designation" value={details.designation}/>
    <Detailsitem key="Email" value={details.email.academic}/>
    <Detailsitem key="Gender" value={details.gender}/>
    <Detailsitem key="Blood Group" value={details.blood_group}/>
</DetailsPage>  
