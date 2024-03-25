<script lang="ts">
	import ListPage from "$lib/SimpleListPage.svelte";
    export let data;
</script>
<ListPage 
        listItems={data.listItems}/>