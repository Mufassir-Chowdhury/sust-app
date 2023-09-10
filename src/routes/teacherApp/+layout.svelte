<script>

	import { AppBar, AppShell, LightSwitch } from "@skeletonlabs/skeleton";

</script>
<AppShell slotSidebarLeft="bg-surface-500/5 w-56 p-4">		

	<svelte:fragment slot="sidebarLeft">
		<!-- Insert the list: -->
		<nav class="list-nav">
			<ul>
				<li><a href="/">Home</a></li>
				<li><a href="/about">About</a></li>
			</ul>
		</nav>
		<!-- --- -->
	</svelte:fragment>

	<!-- Page Route Content -->
	<slot />
</AppShell>
