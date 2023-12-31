<script>
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
    export let data;
    
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item="CSE101">
    <Detailsitem key="Name" value="Intro to Computer Science"/>
    <Detailsitem key="ID" value="CSE101"/>
    <Detailsitem key="Credit" value="3"/>
    <Detailsitem key="Department" value="Computer Science and Engineering"/>
    <Detailsitem key="Type" value="Theory"/>
</DetailsPage>
