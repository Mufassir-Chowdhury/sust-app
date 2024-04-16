<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import List from '$lib/Components/List.svelte';
	import NewButton from '$lib/Components/NewButton.svelte';
    export let data;
</script>
<TitlePage title="Notices">
    <svelte:fragment slot="command">
        <NewButton href="/adminApp/notices/new"/> 
    </svelte:fragment>
    <List listItems={data.listItems} deletable={true} editable={true}/>
</TitlePage>