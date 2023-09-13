<script lang="ts">
	import ListPage from "$lib/ListPage.svelte";
    export let data;
</script>
<ListPage appName={data.appName} pageName={data.pageName} url={data.url} listItems={data.listItems}/>