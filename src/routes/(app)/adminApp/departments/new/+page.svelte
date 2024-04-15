<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";

</script>    
<!-- TODO : add patern and title attributes to form fields -->
<AddForm >
        
    <FormField name="Name" classValue="sm:col-span-3"/>            
    <FormField name="Building" classValue="sm:col-span-3"/>
    <FormField name="Letter Code" classValue="sm:col-span-4"/>
    <FormField name="Code" type="number" classValue="sm:col-span-2 sm:col-start-1"/>
    <FormField name="Floor" type="number" classValue="sm:col-span-2"/>
    <FormField name="Minor Course Code" classValue="sm:col-span-2"/>
</AddForm>