<script lang="ts">

	import { ListBox, ListBoxItem, getModalStore } from '@skeletonlabs/skeleton';
	import type { SvelteComponent } from 'svelte';

	// Props
	/** Exposes parent props to this component. */
	export let parent: SvelteComponent;

	// Local
	let teacher = '';
	const modalStore = getModalStore();

	// Handle Form Submission
	function onFormSubmit(): void {
		if ($modalStore[0].response) $modalStore[0].response(teacher);
		modalStore.close();
	}

	// Base Classes
	const cBase = 'card p-4 w-modal shadow-xl space-y-4';
	const cHeader = 'text-2xl font-bold';
</script>

<!-- @component This example creates a simple form modal. -->

{#if $modalStore[0]}
	<div class="modal-example-form {cBase}">
		<header class={cHeader}>{$modalStore[0].title ?? '(title missing)'}</header>
		<ListBox class="border border-surface-500 p-4 rounded-container-token">
			{#each $modalStore[0].meta as item}
				<ListBoxItem bind:group={teacher} name={item.id} value={item.id}>{item.name}</ListBoxItem>
			{/each}
		</ListBox>
		<!-- prettier-ignore -->
		<footer class="modal-footer {parent.regionFooter}">
        <button class="btn {parent.buttonNeutral}" on:click={parent.onClose}>{parent.buttonTextCancel}</button>
        <button class="btn {parent.buttonPositive}" on:click={onFormSubmit}>Select Teacher</button>
    </footer>
	</div>
{/if}