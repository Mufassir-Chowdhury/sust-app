<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        title={details.name}>
    <Detailsitem key="Name" value={details.name}/>
    <Detailsitem key="ID" value={getID(details.id)}/>
    <Detailsitem key="Building" value={details.building}/>
    <Detailsitem key="Floor" value={details.floor}/>
    <Detailsitem key="Code" value={details.code}/>
    <Detailsitem key="Minor Course Code" value={details.minor_course_code}/>
</DetailsPage>   
