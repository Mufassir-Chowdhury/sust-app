<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;

    let photo = "https://i.pravatar.cc/";
    let dataPairs = [
        {key: "ID", value: getID(details.id)},
        {key: "Department", value: data.department},
        {key: "Designation", value: details.designation},
        {key: "Email", value: details.email.academic},
        {key: "Gender", value: details.gender},
        {key: "Blood Group", value: details.blood_group}
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
