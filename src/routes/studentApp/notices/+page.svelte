<script lang="ts">
	import List from '$lib/Components/List.svelte';
	import TitlePage from '$lib/Page/TitlePage.svelte';
    export let data;
</script>
<TitlePage title="Notices">
    <List listItems={data.listItems}/>
</TitlePage>