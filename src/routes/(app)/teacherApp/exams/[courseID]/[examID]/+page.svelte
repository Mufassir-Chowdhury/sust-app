<script lang="ts">
	import PostDetails from "$lib/Page/PostDetails.svelte";
    
    export let data;
    let details: any = data.details;
</script>
<h3 class="title">{details.title}</h3>
<PostDetails {details} teacher={true} course={data.course.name}/>
