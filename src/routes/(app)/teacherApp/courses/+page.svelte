<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import List from '$lib/Components/List.svelte';
    export let data;
</script>
<TitlePage title="Courses">
    <List listItems={data.listItems}/>
</TitlePage>