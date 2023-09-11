<script lang="ts">
	import AppLayout from "$lib/AppLayout.svelte";
	export let data;

</script>

<AppLayout appName="/{data.appName}" body={data.body}>
	<slot/>
</AppLayout>
