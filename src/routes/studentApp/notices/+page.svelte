<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import SimpleList from '$lib/Components/SimpleList.svelte';
    export let data;
</script>
<TitlePage title="Notices">
    <SimpleList listItems={data.listItems}/>
</TitlePage>