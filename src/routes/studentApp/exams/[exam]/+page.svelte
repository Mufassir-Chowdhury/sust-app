<script lang="ts">
	import { page } from "$app/stores";
	import DetailsPage from "$lib/DetailsPage.svelte";
	import PostDetails from "$lib/PostDetails.svelte";
    
    export let data;
    let details: any = data.details;
    
    let url: string;
    $: url = $page.url.pathname.split('/')[2];
</script>
<DetailsPage 
        url={url}
        title={details.title}>
    <PostDetails {details}/>
</DetailsPage>  
