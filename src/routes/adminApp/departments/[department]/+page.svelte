<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
    import { getID } from "$lib/utils.js";

    export let data;
    let dataPairs = [
        {key: "Name", value: data.details.name},
        {key: "ID", value: getID(data.details.id)},
        {key: "Building", value: data.details.building},
        {key: "Floor", value: data.details.floor},
        {key: "Code", value: data.details.code},
        {key: "Minor Course Code", value: data.details.minor_course_code}
    ];
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={getID(data.details.id)}
        dataPairs={dataPairs}/>   
