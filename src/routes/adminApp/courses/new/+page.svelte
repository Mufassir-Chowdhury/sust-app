<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import SelectField from "$lib/Form/SelectField.svelte";
    export let data;

    const departmentOptions = [
        {name: "Department 1", value: "1"},
        {name: "Department 2", value: "2"},
        {name: "Department 3", value: "3"},
        {name: "Department 4", value: "4"},
        {name: "Department 5", value: "5"},
    ]
    const typeOptions = [
        {name: "Lab", value: "lab"},
        {name: "Theory", value: "theory"},
    ]

</script>    
<!-- TODO : add patern and title attributes -->
<AddForm 
        appName={data.appName} 
        url={data.url} 
        pageName={data.pageName} 
        appUrl={data.appUrl}>
        
    <FormField name="Name" classValue="sm:col-span-3"/>            
    <FormField name="Credit" type="number" classValue="sm:col-span-3"/>
    <SelectField name="Department" classValue="sm:col-span-4" options={departmentOptions}/>
    <SelectField name="Type" classValue="sm:col-span-2" options={typeOptions}/>
</AddForm>