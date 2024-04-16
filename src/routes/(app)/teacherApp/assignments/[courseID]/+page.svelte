<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import List from '$lib/Components/List.svelte';
	import { page } from '$app/stores';
	import { getID } from '$lib/utils.js';
	import NewButton from '$lib/Components/NewButton.svelte';
    export let data;
</script>
<TitlePage title="Assignments for {getID($page.params.courseID)}">
	<svelte:fragment slot="command">
        <NewButton href="/teacherApp/assignments/{$page.params.courseID}/new"/> 
    </svelte:fragment>
    <List listItems={data.listItems} slug={`/teacherApp/assignments/${$page.params.courseID}`} deletable={true}/>
</TitlePage>