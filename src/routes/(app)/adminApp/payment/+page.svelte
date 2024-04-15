<script lang="ts">
    import { Table, tableMapperValues } from '@skeletonlabs/skeleton';
    import type { TableSource } from '@skeletonlabs/skeleton';

    const sourceData = [
        { position: 1, name: 'Hydrogen', weight: 1.0079, symbol: 'H' },
        { position: 2, name: 'Helium', weight: 4.0026, symbol: 'He' },
        { position: 3, name: 'Lithium', weight: 6.941, symbol: 'Li' },
        { position: 4, name: 'Beryllium', weight: 9.0122, symbol: 'Be' },
        { position: 5, name: 'Boron', weight: 10.811, symbol: 'B' },
    ];
    
const tableSimple: TableSource = {
	// A list of heading labels.
	head: ['Name', 'Symbol', 'Weight'],
	// The data visibly shown in your table body UI.
	body: tableMapperValues(sourceData, ['name', 'symbol', 'weight']),
	// Optional: The data returned when interactive is enabled and a row is clicked.
	meta: tableMapperValues(sourceData, ['position', 'name', 'symbol', 'weight']),
	// Optional: A list of footer labels.
	foot: ['Total', '', '<code class="code">5</code>']
};
				                   

</script>

<div class="p-8">
    

    <h2 class="h2">
        Payment
    </h2>
    <div class="py-8">
        <div class="mx-auto max-w-7xl px-6 :px-8">
          <dl class="grid gap-x-8 gap-y-16 text-center grid-cols-4">
            <div class="mx-auto flex max-w-xs flex-col gap-y-4">
                <dd class="order-first text-3xl font-semibold tracking-tight text-gray-900 sm:text-5xl">৳10,000</dd>
                <dt class="text-base leading-7 text-gray-600">Amount of transactions in this semester</dt>
            </div>
            <div class="mx-auto flex max-w-xs flex-col gap-y-4">
                <dd class="order-first text-3xl font-semibold tracking-tight text-gray-900 sm:text-5xl">৳500</dd>
                <dt class="text-base leading-7 text-gray-600">Number of transactions</dt>
            </div>
            <div class="mx-auto flex max-w-xs flex-col gap-y-4">
                <dd class="order-first text-3xl font-semibold tracking-tight text-gray-900 sm:text-5xl">৳46,000</dd>
                <dt class="text-base leading-7 text-gray-600">Total amount of transactions in this semester</dt>
            </div>
            <div class="mx-auto flex max-w-xs flex-col gap-y-4">
                <dd class="order-first text-3xl font-semibold tracking-tight text-gray-900 sm:text-5xl">৳46,000</dd>
                <dt class="text-base leading-7 text-gray-600">Total number of transactions in this semester</dt>
            </div>
          </dl>
        </div>
      </div>
      <Table source={tableSimple} interactive={true} />      
</div>
