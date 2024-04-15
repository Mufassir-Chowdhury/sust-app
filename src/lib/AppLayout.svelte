<script lang="ts">
	import { routes } from './routes';
	import { AppShell, ListBox, ListBoxItem } from "@skeletonlabs/skeleton";
	import { page } from '$app/stores';
	import type { Group } from "$lib/models";
	
	export let body: Group[];

    let appName: string;
	$: appName = $page.url.pathname.split('/')[1];
	let url: string[];
	$: url = $page.url.pathname.split('/')
								.filter((path: string) => {return path != ""})
								.slice(0, -1);
							
</script>
{#if $page.url.pathname.split('/').length !== 2}
<AppShell slotSidebarLeft="bg-surface-500/5 w-64 p-4">	
	
	
		<svelte:fragment slot="sidebarLeft">
			<!-- Navigation list: -->
			<nav class="list-nav" >
				<ListBox spacing="space-y-0">
					<div class="w-full flex justify-between border-b-2 border-b-primary-400">

						<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={appName} name="nav" value='home'>
							<svelte:fragment slot="lead"><i class="fi fi-tr-house-chimney pl-4"></i></svelte:fragment>
								<div class="flex justify-between w-full">		
									<a href='/{appName}'>
										Home
									</a>
								</div>	
								
						</ListBoxItem>
						<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={appName} name="nav" value='logout'>
							<svelte:fragment slot="lead" >
								<div class="pl-2 ">
									<svg fill="#000000"  height="16px" width="16px" version="1.1" id="Capa_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 384.971 384.971" xml:space="preserve"><g><g id="Sign_Out"> <path d="M180.455,360.91H24.061V24.061h156.394c6.641,0,12.03-5.39,12.03-12.03s-5.39-12.03-12.03-12.03H12.03 C5.39,0.001,0,5.39,0,12.031V372.94c0,6.641,5.39,12.03,12.03,12.03h168.424c6.641,0,12.03-5.39,12.03-12.03 C192.485,366.299,187.095,360.91,180.455,360.91z"/> <path d="M381.481,184.088l-83.009-84.2c-4.704-4.752-12.319-4.74-17.011,0c-4.704,4.74-4.704,12.439,0,17.179l62.558,63.46H96.279 c-6.641,0-12.03,5.438-12.03,12.151c0,6.713,5.39,12.151,12.03,12.151h247.74l-62.558,63.46c-4.704,4.752-4.704,12.439,0,17.179 c4.704,4.752,12.319,4.752,17.011,0l82.997-84.2C386.113,196.588,386.161,188.756,381.481,184.088z"/></g><g></g><g></g><g></g><g></g><g></g><g></g></g></svg>							
								</div>
							</svelte:fragment>
							<form method="POST" action="/?/logout">
								<button type='submit' class="btn">Log out</button>
							</form>

								
						</ListBoxItem>
					</div>
					{#each body as pageGroups}
						{#each pageGroups.pages as page}
							<ListBoxItem class="block min-w-full"   padding="p-0" bind:group={appName} name="nav" value={page}>
								<svelte:fragment slot="lead"><i class="fi {routes[page].icon} pl-4"></i></svelte:fragment>
									<a href='/{appName}/{page}'>
										{routes[page].title}
									</a>
							</ListBoxItem>
						{/each}
					{/each}
				</ListBox>
			</nav>
			<!-- --- -->
		</svelte:fragment>
		
		<!-- Page Route Content -->
		<div class="p-8">
		{#if url.length > 1}
				<ol class="breadcrumb mb-8">
					{#each url as urlItem, index}
						<li class="crumb"><a class="anchor" href="/{url.slice(0, index+1).join("/")}">{routes[urlItem]?.title || urlItem}</a></li>
						<li class="crumb-separator" aria-hidden>/</li>
						
					{/each}

					<li>{$page.url.pathname.split('/').at(-1)}</li>
				</ol>
				{/if}
				<slot />
		</div>
</AppShell>
{:else}
	<AppShell>
		<slot/>
	</AppShell>
{/if}
