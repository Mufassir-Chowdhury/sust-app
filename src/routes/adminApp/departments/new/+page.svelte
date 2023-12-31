<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
    import type { ActionData } from "./$types.js";
    export let data;

    export let form: ActionData;
</script>    
<!-- TODO : add patern and title attributes -->
<AddForm 
        appName={data.appName} 
        url={data.url} 
        pageName={data.pageName} 
        appUrl={data.appUrl}>
        
    <FormField name="Name" classValue="sm:col-span-3"/>            
    <FormField name="Building" classValue="sm:col-span-3"/>
    <FormField name="Letter Code" classValue="sm:col-span-4"/>
    <FormField name="Code" type="number" classValue="sm:col-span-2 sm:col-start-1"/>
    <FormField name="Floor" type="number" classValue="sm:col-span-2"/>
    <FormField name="Minor Course Code" classValue="sm:col-span-2"/>
</AddForm>
{#if form?.body.message }
    {form?.body.message}
{/if}