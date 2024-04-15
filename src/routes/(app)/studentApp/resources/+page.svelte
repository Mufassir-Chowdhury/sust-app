<script>
	const items = [
		{ id: 1, name: 'Notes', url: '/studentApp/resources/notes' },
		{ id: 2, name: 'Videos', url: '/studentApp/resources/videos' },
		{ id: 3, name: 'Questions', url: '/studentApp/resources/questions' },
		{ id: 4, name: 'item 4', url: '/studentApp/resources/item4' },
		{ id: 5, name: 'item 5', url: '/studentApp/resources/item5' }
	];
</script>

<div class="flex items-center justify-center my-auto">
	<div
		class="container grid grid-cols-3 md:grid-cols-4 gap-6 p-4 items-center justify-center my-auto"
	>
		{#each items as item}
			<a
				href={item.url}
				class="card cursor-pointer card-hover variant-soft-surface flex flex-col p-8 items-center justify-center gap-4"
			>
				<i class="fi fi-tr-clipboard" />
				{item.name}
			</a>
		{/each}
	</div>
</div>
