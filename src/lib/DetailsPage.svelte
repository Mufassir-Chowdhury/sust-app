<script lang="ts">
	import { Avatar } from "@skeletonlabs/skeleton";

    class Item{
        key: string;
        value: string;
        constructor(key: string, value: string){
            this.key = key;
            this.value = value;
        }
    }
    export let appUrl: String;
    export let pageName: String;
    export let url: String;
    export let appName: String;
    export let item: string;
    export let details: Item[];
    export let photo: string;
    export let name: string;
</script>

<div class="p-8">
    <ol class="breadcrumb col-span-full mb-8">
        <li class="crumb"><a class="anchor" href="/{appUrl}">{appName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li class="crumb"><a class="anchor" href="/{appUrl}/{url}">{pageName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li>{item}</li>
    </ol>
    <div class="mt-6 border-t border-gray-100">
        <div class="flex flex-col items-center">
            <Avatar src={photo} />     
            <dd class="text-xl font-semibold leading-6 text-gray-700 mt-0 mb-8">{name}</dd>
        </div>
      <dl class="divide-y">

        {#each details as detail}
            <div class="py-4 grid grid-cols-3 gap-4 px-0">
                <dt class="text-sm font-medium leading-6 text-gray-900">{detail.key}</dt>
                <dd class="text-sm leading-6 text-gray-700 col-span-2 mt-0">{detail.value}</dd>
            </div>
        {/each}
        
      </dl>
    </div>
  </div>
  