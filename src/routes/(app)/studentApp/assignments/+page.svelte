<script lang="ts">
	import List from '$lib/Components/List.svelte';
	import TitlePage from '$lib/Page/TitlePage.svelte';
    export let data;
</script>
<TitlePage title="Assignemnts">
    <List listItems={data.listItems}/>
</TitlePage>