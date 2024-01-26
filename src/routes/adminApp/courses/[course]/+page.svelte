<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
    let dataPair = [
        {key: "Name", value: details.name},
        {key: "ID", value: getID(details.id)},
        {key: "Credit", value: details.credit},
        {key: "Department", value: data.department},
        {key: "Type", value: details.type}
    ];
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={getID(details.id)}
        dataPairs={dataPair}/>
