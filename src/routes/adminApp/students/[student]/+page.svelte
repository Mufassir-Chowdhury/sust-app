<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;

    let photo = "https://i.pravatar.cc/";
    
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={details.name}
        photo={photo}
        name={details.name}>
    <Detailsitem key="ID" value={getID(details.id)}/>
    <Detailsitem key="Department" value={data.department}/>
    <Detailsitem key="Email" value={details.email.academic}/>
    <Detailsitem key="Gender" value={details.gender}/>
    <Detailsitem key="Blood Group" value={details.blood_group}/>
    <Detailsitem key="Current Semester" value={details.current_semester}/>
    <Detailsitem key="Session" value={details.session}/>
</DetailsPage>
