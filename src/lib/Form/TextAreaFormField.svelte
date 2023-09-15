<script lang="ts">
    export let name: string;
    export let classValue: string = "";
    const id = name.replaceAll(' ', '-').toLowerCase();
</script>

<label class="label {classValue}">
    <span class="text-sm font-medium text-gray-900 leading-6">{name}</span>
    <div class="flex rounded-md shadow-sm ring-1 ring-inset ring-gray-400 focus-within:ring-2 focus-within:ring-inset focus-within:ring-indigo-600 sm:max-w-md">
        <textarea id={id} name={id} placeholder={name} rows="3" class="block bg-transparent w-full rounded-md border-0 py-1.5 text-gray-900 shadow-sm ring-1 ring-inset ring-gray-400 placeholder:text-gray-400 focus:ring-2 focus:ring-inset focus:ring-indigo-600 sm:text-sm sm:leading-6"></textarea>
      </div>
</label>