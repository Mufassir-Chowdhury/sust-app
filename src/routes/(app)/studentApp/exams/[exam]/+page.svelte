<script lang="ts">
	import PostDetails from "$lib/Page/PostDetails.svelte";
    
    export let data;
    let details: any = data.details;
    console.log(data.course[0])
</script>
<h3 class="title">{details.title}</h3>
<PostDetails {details} course={data.course[0].name}/>
