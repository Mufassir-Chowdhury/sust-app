<script>
	import Dashboard from "./Dashboard.svelte";
	import NavigationGrid from "./NavigationGrid.svelte";

</script>


<div class="grid grid-cols-3 px-8 h-screen">
    <div class="col-span-2 h-full">
        <NavigationGrid/>
    </div>
    <div class="col-span-1">
        <Dashboard/>
    </div>
</div>