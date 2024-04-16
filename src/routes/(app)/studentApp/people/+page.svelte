<script lang="ts">
	import TitlePage from '$lib/Page/TitlePage.svelte';
	import List from '$lib/Components/List.svelte';
    import { TabGroup, Tab, TabAnchor } from '@skeletonlabs/skeleton';
    let tabSet: number = 0;
    export let data;
</script>
<TitlePage title="People">
    <TabGroup justify="justify-center">
        <Tab bind:group={tabSet} name="tab1" value={0} class="px-28">Students</Tab>
        <Tab bind:group={tabSet} name="tab2" value={1} class="px-28">Teachers</Tab>
        <!-- Tab Panels --->
        <svelte:fragment slot="panel">
            {#if tabSet === 0}
                <List listItems={data.students}/>

            {:else if tabSet === 1}
                <List listItems={data.teachers}/>
            {/if}
        </svelte:fragment>
    </TabGroup>
</TitlePage>