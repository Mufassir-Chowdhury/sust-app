<script>
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import SelectField from "$lib/Form/SelectField.svelte";
	import TextAreaFormField from "$lib/Form/TextAreaFormField.svelte";
	import PhotoPicker from "$lib/Form/PhotoPicker.svelte";
	import { bloodGroupOptions, genderOptions } from "$lib/utils";
  export let data;

</script>    
<!-- TODO : add patern and title attributes -->
<AddForm >
    <!-- <svelte:fragment slot="profile">
      <PhotoPicker/>
      <TextAreaFormField name="About" classValue="col-span-full"/>
    </svelte:fragment> -->
    <FormField name="Name" classValue="sm:col-span-3"/>   
    <SelectField name="Department" options={data.departmentOptions} classValue="sm:col-span-3" />  
    <FormField name="Designation" classValue="sm:col-span-4 col-start-1"/>          
    <SelectField name="Blood Group" options={bloodGroupOptions} classValue="sm:col-span-1" />         
    <SelectField name="Gender" options={genderOptions} classValue="sm:col-span-1" />         
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