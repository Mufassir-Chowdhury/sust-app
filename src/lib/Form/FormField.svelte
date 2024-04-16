<script lang="ts">
    export let name: string;
    export let classValue: string = "";
    export let type: string = "text";
    export let leading: string = "";
    export let required: boolean = true;
    const id = name.replaceAll(' ', '-').toLowerCase();
</script>

<label class="label {classValue} {type=="checkbox" ? "flex gap-4" : ""}">
    <span class="text-sm font-medium text-gray-900 leading-6">{name}</span>
    <div class="flex rounded-md shadow-sm ring-1 ring-inset ring-gray-400 focus-within:ring-2 focus-within:ring-inset focus-within:ring-indigo-600 sm:max-w-md">
        {#if leading != ""}
            <span class="flex select-none items-center pl-3 text-gray-500 sm:text-sm">{leading}:</span>
        {/if}
        <input {required} type={type} name={id} id={id} autocomplete={id} class="block flex-1 border-0 bg-transparent {type=='checkbox' ? 'py-1.5 pl-3' : ''} text-gray-900 placeholder:text-gray-400 focus:ring-0 sm:text-sm sm:leading-6" placeholder={name}>
      </div>
</label>