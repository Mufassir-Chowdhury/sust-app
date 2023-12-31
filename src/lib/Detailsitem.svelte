<script lang="ts">
    export let key: string;
    export let value: string;
</script>

<div class="py-4 grid grid-cols-3 gap-4 px-0">
    <dt class="text-sm font-medium leading-6 text-gray-900">{key}</dt>
    <dd class="text-sm leading-6 text-gray-700 col-span-2 mt-0">{value}</dd>
</div>