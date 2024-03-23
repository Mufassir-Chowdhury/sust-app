<script lang="ts">
	import AppLayout from "$lib/AppLayout.svelte";
	export let data;

</script>

<AppLayout appName="/{data.appUrl}" body={data.body}>
	<slot/>
</AppLayout>
