<script>
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
    export let data;

    let photo = "https://i.pravatar.cc/";
    let name = 'Mufassir Ahmad Chowdhury';
    
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item="Mufassir Ahmad Chowdhury"
        photo={photo}
        name={name}>
    <Detailsitem key="ID" value="2019331073"/>
    <Detailsitem key="Department" value="Computer Science and Engineering"/>
    <Detailsitem key="Email" value="mufassir73@student.sust.edu"/>
    <Detailsitem key="Gender" value="Male"/>
    <Detailsitem key="Blood Group" value="B+"/>
    <Detailsitem key="Session" value="2019"/>
    <Detailsitem key="Current Semester" value="3"/>
</DetailsPage>
