<script lang="ts">
	import { routes } from './../routes';
	import FormField from "$lib/Form/FormField.svelte";
	import { page } from '$app/stores';
    let appUrl: string;
    $: appUrl = $page.url.pathname.split('/')[1];
    let url: string;
    $: url = $page.url.pathname.split('/')[2];
    export let ID = true;
</script>
<div class="grid grid-cols-6">

    <form class="col-start-2 col-span-4" method="POST">
        {#if ID}
            <fieldset class="border-b border-gray-900/10 pb-12 col-span-full">
                <h2 class="text-base font-semibold leading-7 text-gray-900">{routes[url].title}</h2>
                    <FormField name="ID" leading={routes[url].title.toLowerCase()} classValue="sm:col-span-4 mt-10"/>
                    <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
                        <slot name="profile" />
                    </div>
                </fieldset>
        {/if}
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