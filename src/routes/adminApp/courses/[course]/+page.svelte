<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={getID(details.id)}
        title={details.name}>
    <Detailsitem key="Name" value={details.name}/>
    <Detailsitem key="ID" value={getID(details.id)}/>
    <Detailsitem key="Credit" value={details.credit}/>
    <Detailsitem key="Department" value={data.department}/>
    <Detailsitem key="Type" value={details.type}/>
</DetailsPage>
