<script lang="ts">
	import AppLayout from "$lib/AppLayout.svelte";
	export let data;

</script>

<AppLayout  body={data.body}>
	<slot/>
</AppLayout>
