<script lang="ts">
	import { Avatar } from "@skeletonlabs/skeleton";

    export let appUrl: String;
    export let pageName: String;
    export let url: String;
    export let appName: String;
    export let item: string;
    export let photo: string= "";
    export let name: string= "";
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
        {#if photo != ""}
            <div class="flex flex-col items-center">
                <Avatar src={photo} />     
                <dd class="text-xl font-semibold leading-6 text-gray-700 mt-0 mb-8">{name}</dd>
            </div>
        {/if}
      <dl class="divide-y">
        <slot/>
      </dl>
    </div>
  </div>
  