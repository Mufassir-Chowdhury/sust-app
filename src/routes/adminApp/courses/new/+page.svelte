<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import SelectField from "$lib/Form/SelectField.svelte";
	import { typeOptions } from "$lib/utils.js";
    export let data;

</script>    
<!-- TODO : add patern and title attributes -->
<AddForm >
    <FormField name="Name" classValue="sm:col-span-3"/>            
    <FormField name="Credit" type="float" classValue="sm:col-span-3"/>
    <SelectField name="Department" classValue="sm:col-span-4" options={data.departmentOptions}/>
    <SelectField name="Type" classValue="sm:col-span-2" options={typeOptions}/>
    <FormField name="Year" type="number" classValue="sm:col-span-3"/>
    <FormField name="Semester" type="number" classValue="sm:col-span-3"/>

</AddForm>