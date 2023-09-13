<script lang="ts">
	import FormField from "$lib/FormField.svelte";
    export let appUrl: String;
    export let pageName: String;
    export let url: String;
    export let appName: String;
    let singularPageName = pageName.slice(0, -1);
</script>
<div class="p-8 grid grid-cols-6">

    <ol class="breadcrumb col-span-full mb-8">
            
        <li class="crumb"><a class="anchor" href="/{appUrl}">{appName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li class="crumb"><a class="anchor" href="/{appUrl}/{url}">{pageName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li>New</li>
    </ol>

    <form class="col-start-2 col-span-4">
        <fieldset class="border-b border-gray-900/10 pb-12">
            <h2 class="text-base font-semibold leading-7 text-gray-900">{singularPageName}</h2>
            <FormField name="ID" leading={singularPageName.toLowerCase()} classValue="sm:col-span-4 mt-10"/>
        </fieldset>
        <fieldset class="mt-6 border-b border-gray-900/10 pb-12">
            <h2 class="text-base font-semibold leading-7 text-gray-900">Details</h2>
            <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
                <slot/>
            </div>
        </fieldset>         
        <fieldset class="mt-6 flex items-center justify-end gap-x-6">
            <a class="btn !bg-transparent" href="#">Cancel</a>
            <a class="btn variant-filled-primary" href="/{appUrl}/{url}">Create</a>
        </fieldset>
    </form>
</div>