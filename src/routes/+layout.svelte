<script lang="ts">
	import '../app.postcss';
	import { AppShell, AppBar } from '@skeletonlabs/skeleton';

	// Highlight JS
	import hljs from 'highlight.js';
	import 'highlight.js/styles/github-dark.css';
	import { storeHighlightJs } from '@skeletonlabs/skeleton';
	storeHighlightJs.set(hljs);

	// Floating UI for Popups
	import { computePosition, autoUpdate, flip, shift, offset, arrow } from '@floating-ui/dom';
	import { storePopup } from '@skeletonlabs/skeleton';
	storePopup.set({ computePosition, autoUpdate, flip, shift, offset, arrow });

	import { LightSwitch } from '@skeletonlabs/skeleton';

	
	import { page } from '$app/stores';
	import { onMount } from 'svelte';
	import Swal from 'sweetalert2';
	import { routes } from '$lib/routes';
	let currentRoute = '';
	$: currentRoute = $page.route?.id ?? ''

	const hanndleHome = () => {
		//if the url contains adminApp then redirect to adminApp
		if (currentRoute.includes('adminApp')) {
			window.location.href = '/adminApp';
		}
		//if the url contains studentApp then redirect to studentApp
		else if (currentRoute.includes('studentApp')) {
			window.location.href = '/studentApp';
		}
		//if the url contains teacherApp then redirect to teacherApp
		else if (currentRoute.includes('teacherApp')) {
			window.location.href = '/teacherApp';
		}

	}

	onMount(() => {
      if (new URL(window.location.href).searchParams.get('showSuccess') === 'true') {
          Swal.fire({
              title: "Successful!",
              text: `New ${routes[$page.url.pathname.split('/')[2]].title} is registered!`,
              icon: "success"
          });
      }
  });
</script>

<!-- App Shell -->
<!-- svelte-ignore missing-declaration -->
<AppShell slotSidebarLeft="bg-surface-500/5 w-56  h-screen">
	<div class="absolute invisible">
		<LightSwitch />
	</div>	
	<!-- <svelte:fragment slot="header">
		<AppBar>
			<svelte:fragment slot="lead">
				<button class="text-xl uppercase font-medium" on:click={hanndleHome} on:keydown={hanndleHome}>SUST APP</button>
			</svelte:fragment>
			<svelte:fragment slot="trail">
				<LightSwitch />
				{#if currentRoute !== '/'}
				<a href="/"><div class="btn variant-filled-primary">Logout</div></a>
				{/if}
			</svelte:fragment>
		</AppBar>
	</svelte:fragment> -->

	<!-- Page Route Content -->
	<slot />
</AppShell>
