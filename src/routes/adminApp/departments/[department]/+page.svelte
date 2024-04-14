<script lang="ts">
	import Detailsitem from "$lib/Components/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<h3 class="title">{details.title ?? details.name}</h3>
<Detailsitem key="Name" value={details.title ?? details.name}/>
<Detailsitem key="ID" value={getID(details.id)}/>
<Detailsitem key="Building" value={details.building}/>
<Detailsitem key="Floor" value={details.floor}/>
<Detailsitem key="Code" value={details.code}/>
<Detailsitem key="Minor Course Code" value={details.minor_course_code}/>
