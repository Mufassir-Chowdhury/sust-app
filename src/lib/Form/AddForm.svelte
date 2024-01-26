<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
    export let appUrl: String;
    export let pageName: String;
    export let url: String;
    export let appName: String;
    
</script>
<div class="p-8 grid grid-cols-6">

    <ol class="breadcrumb col-span-full mb-8">
            
        <li class="crumb"><a class="anchor" href="/{appUrl}">{appName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li class="crumb"><a class="anchor" href="/{appUrl}/{url}">{pageName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li>New</li>
    </ol>

    <form class="col-start-2 col-span-4" method="POST">
        <fieldset class="border-b border-gray-900/10 pb-12 col-span-full">
            <h2 class="text-base font-semibold leading-7 text-gray-900">{pageName}</h2>
            <FormField name="ID" leading={pageName.toLowerCase()} classValue="sm:col-span-4 mt-10"/>
            <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
                <slot name="profile" />
            </div>
        </fieldset>
        <fieldset class="mt-6 border-b border-gray-900/10 pb-12">
            <h2 class="text-base font-semibold leading-7 text-gray-900">Details</h2>
            <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
                <slot/>
            </div>
        </fieldset> 
        {#if $$slots.personal}
            <fieldset class="mt-6 border-b border-gray-900/10 pb-12">
                <h2 class="text-base font-semibold leading-7 text-gray-900">Personal Information</h2>
                <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
                    <slot name="personal"/>
                </div>
            </fieldset>         
        {/if}
        <fieldset class="mt-6 flex items-center justify-end gap-x-6">
            <a class="btn !bg-transparent" href="/{appUrl}/{url}">Cancel</a>
            <button type="submit" class="btn variant-filled-primary">Create</button>
        </fieldset>
    </form>
</div>