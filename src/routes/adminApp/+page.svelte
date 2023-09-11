<script lang="ts">
	import Dashboard from "$lib/Dashboard.svelte";
	import NavigationGrid from "$lib/NavigationGrid.svelte";

    export let data;
</script>


<div class="grid grid-cols-3 px-8 h-screen">
    <div class="col-span-2 h-full">
        <NavigationGrid pages={data.body} app='adminApp'/>
    </div>
    <div class="col-span-1">
        <Dashboard/>
    </div>
</div>