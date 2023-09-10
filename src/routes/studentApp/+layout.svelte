<script lang="ts">
	class NavItem {
		constructor(name: string, url: string) {
			this.name = name;
			this.url = url;
		}
		name: string;
		url: string;
	}

	import { AppShell, ListBox, ListBoxItem } from "@skeletonlabs/skeleton";
	import { page } from '$app/stores';

	$: nav = $page.route.id ?? "/studentApp";

	let pages: NavItem[] = [
		new NavItem("Home", "/studentApp"),
		new NavItem("Routine", "/studentApp/routine"),
		new NavItem("Updates", "/studentApp/posts"),
		new NavItem("Course Registration", "/studentApp/course-registration"),
		new NavItem("Departments", "/studentApp/department"),
	];
</script>
{#if $page.route.id !== "/studentApp"}
<AppShell slotSidebarLeft="bg-surface-500/5 w-64 p-4">		
		<svelte:fragment slot="sidebarLeft">
			<!-- Navigation list: -->
			<nav class="list-nav" >
				<ListBox spacing="space-y-4">
					{#each pages as page}
						<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={nav} name="nav" value={page.url}>
							<a href={page.url}>
									{page.name}
							</a>
						</ListBoxItem>
					{/each}
				</ListBox>
			</nav>
			<!-- --- -->
		</svelte:fragment>
		
		<!-- Page Route Content -->
		<slot />
</AppShell>
{:else}
	<AppShell>
		<slot/>
	</AppShell>
{/if}
