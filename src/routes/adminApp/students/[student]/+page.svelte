<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;

    let photo = "https://i.pravatar.cc/";
    let dataPairs = [
        {key: "ID", value: getID(details.id)},
        {key: "Department", value: data.department},
        {key: "Email", value: details.email.academic},
        {key: "Gender", value: details.gender},
        {key: "Blood Group", value: details.blood_group},
        {key: "Current Semester", value: details.current_semester},
        {key: "Session", value: details.session}
    ]
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={details.name}
        photo={photo}
        name={details.name}
        dataPairs={dataPairs}/>
