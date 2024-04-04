<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import List from '$lib/Components/List.svelte';
	import NewButton from '$lib/Components/NewButton.svelte';
    export let data;
</script>
<TitlePage title="Teachers">
    <svelte:fragment slot="command">
        <NewButton href="/adminApp/teachers/new"/> 
    </svelte:fragment>
    <List listItems={data.listItems}/>
</TitlePage>