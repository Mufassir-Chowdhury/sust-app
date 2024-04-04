<script lang="ts">
	import Detailsitem from "$lib/Components/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
    
</script>
<h3 class="title">{details.name}</h3>
<Detailsitem key="Name" value={details.name}/>
<Detailsitem key="ID" value={getID(details.id)}/>
<Detailsitem key="Credit" value={details.credit}/>
<Detailsitem key="Department" value={data.department}/>
<Detailsitem key="Type" value={details.type}/>
