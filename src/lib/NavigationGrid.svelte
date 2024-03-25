<script lang="ts">
	import { routes } from './routes';
	import type { Group } from '$lib/models.ts';

	export let pages: Group[];
	export let app: String;
</script>

<div class="h-screen grid grid-rows-4 gap-4 p-4">
	
	{#each pages as page}
		<div class="">
			<h1 class="text-left">
				{page.title}
			</h1>
			<div class="grid grid-cols-4 h-full">
				{#each page.pages as item}
					<a
						href="\{app}\{item}"
						class="card variant-soft-surface text-center col-span-1 m-4 flex flex-col justify-center"
					>
						<i class="fi {routes[item].icon}" />
						{routes[item].title}
					</a>
				{/each}
			</div>
		</div>
	{/each}
</div>
