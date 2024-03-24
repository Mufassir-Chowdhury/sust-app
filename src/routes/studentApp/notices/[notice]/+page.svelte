<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import PostDetails from "$lib/PostDetails.svelte";
    
    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        url={data.url}
        appUrl={data.appUrl}
        item={details.title}
        title={details.title}>
    <PostDetails {details}/>
</DetailsPage>  
