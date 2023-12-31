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
        item="CSE">
    <Detailsitem key="Name" value="Computer Science and Engineering"/>
    <Detailsitem key="ID" value="CSE"/>
    <Detailsitem key="Building" value="IICT Building"/>
    <Detailsitem key="Floor" value="3rd"/>
    <Detailsitem key="Code" value="331"/>
    <Detailsitem key="Minor Course Code" value="D"/>    
</DetailsPage>
