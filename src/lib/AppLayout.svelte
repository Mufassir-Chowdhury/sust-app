<script lang="ts">
	import { AppShell, ListBox, ListBoxItem } from "@skeletonlabs/skeleton";
	import { page } from '$app/stores';
	import type { Group } from "$lib/models";

    export let appName: string;
	export let body: Group[];


</script>
{#if $page.route.id !== appName}
<AppShell slotSidebarLeft="bg-surface-500/5 w-64 p-4">		
		<svelte:fragment slot="sidebarLeft">
			<!-- Navigation list: -->
			<nav class="list-nav" >
				<ListBox spacing="space-y-4">
					<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={appName} name="nav" value='home'>
						<svelte:fragment slot="lead"><i class="fi fi-tr-house-chimney pl-4"></i></svelte:fragment>
							<a href='{appName}'>
								Home
							</a>
					</ListBoxItem>
					{#each body as pageGroups}
						{#each pageGroups.pages as page}
							<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={appName} name="nav" value={page.url}>
								<svelte:fragment slot="lead"><i class="fi {page.icon} pl-4"></i></svelte:fragment>
									<a href='{appName}{page.url}'>
										{page.name}
									</a>
							</ListBoxItem>
						{/each}
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
