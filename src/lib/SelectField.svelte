<script lang="ts">
    export let name: string;
    export let classValue: string = "";
    const id = name.replaceAll(' ', '-').toLowerCase();
    class Option{
        value: string;
        name: string;
        constructor(value: string, name: string){
            this.value = value;
            this.name = name;
        }
    }
    export let options: Option[] = [];
</script>

<label class="label {classValue}">
    <span class="text-sm font-medium text-gray-900 leading-6">{name}</span>
    <div class="flex rounded-md shadow-sm ring-1 ring-inset ring-gray-400 focus-within:ring-2 focus-within:ring-inset focus-within:ring-indigo-600 sm:max-w-md">
        <select required id="{name.toLowerCase()}" class="select block flex-1 border-0 bg-transparent py-1.5 pl-3 text-gray-900 placeholder:text-gray-400 focus:ring-0 sm:text-sm sm:leading-6">
            {#each options as option}
                <option value="{option.value}">{option.name}</option>
            {/each}
        </select>
    </div>
</label>