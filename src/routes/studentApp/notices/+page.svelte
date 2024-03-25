<script lang="ts">
	import ListPage from "$lib/ListPage.svelte";
    export let data;
</script>
<ListPage 
        listItems={data.listItems}/>