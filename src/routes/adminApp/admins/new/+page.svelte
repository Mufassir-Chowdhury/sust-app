<script>
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import SelectField from "$lib/Form/SelectField.svelte";
	import TextAreaFormField from "$lib/Form/TextAreaFormField.svelte";
	import PhotoPicker from "$lib/Form/PhotoPicker.svelte";
  export let data;

  const departmentOptions = [
      {name: "Department 1", value: "1"},
      {name: "Department 2", value: "2"},
      {name: "Department 3", value: "3"},
      {name: "Department 4", value: "4"},
      {name: "Department 5", value: "5"},
  ]
</script>    
<!-- TODO : add patern and title attributes -->
<AddForm 
        appName={data.appName} 
        url={data.url} 
        pageName={data.pageName} 
        appUrl={data.appUrl}>
    <svelte:fragment slot="profile">
      <PhotoPicker/>
      <TextAreaFormField name="About" classValue="col-span-full"/>
    </svelte:fragment>
    <FormField name="Name" classValue="sm:col-span-3"/>   
    <SelectField name="Department" options={departmentOptions} classValue="sm:col-span-3" />  
    <FormField name="Designation" classValue="sm:col-span-4 col-start-1"/>          
    <SelectField name="Blood Group" options={departmentOptions} classValue="sm:col-span-2" />         
    <FormField name="Personal Email" type="email" classValue="sm:col-span-3"/>
    <FormField name="Academic Email" type="email" classValue="sm:col-span-3"/>
    <svelte:fragment slot="personal">
      <FormField name="Father's Name" classValue="sm:col-span-3"/>
      <FormField name="Mother's Name" classValue="sm:col-span-3"/>
      <FormField name="Date Of Birth" type="date" classValue="sm:col-span-2"/>
      <FormField name="Phone" type="phone" classValue="sm:col-span-2"/>
      <FormField name="Hometown" classValue="sm:col-span-2"/>
    </svelte:fragment>

  </AddForm>