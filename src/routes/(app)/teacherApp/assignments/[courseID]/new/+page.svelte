<script>
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import TextAreaFormField from "$lib/Form/TextAreaFormField.svelte";

</script>    
<!-- TODO : add pattern and title attributes -->
<AddForm ID={false}>
    <FormField name="Title" classValue="sm:col-span-3"/>   
    <FormField name="Total Grade" type="number" classValue="sm:col-span-3"/>
    <FormField name="Due Date" type="date" classValue="sm:col-span-2"/>
    <TextAreaFormField name="Details" classValue="col-span-full"/>
    <div>
        <FormField name="Submission" type="checkbox" classValue="sm:col-span-3"/>
    </div>
  </AddForm>