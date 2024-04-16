<script lang="ts">
	import FormField from "$lib/Form/FormField.svelte";
	import AddForm from "$lib/Form/AddForm.svelte";
	import TextAreaFormField from "$lib/Form/TextAreaFormField.svelte";
	import { ListBox, ListBoxItem } from "@skeletonlabs/skeleton";
    let valueMultiple: string[] = [];
    export let data;
</script>    
<!-- TODO : add patern and title attributes to form fields -->
<AddForm ID={false}>
        
    <FormField name="Title" classValue="sm:col-span-3"/>            
    <FormField name="Creator" classValue="sm:col-span-3"/>
    <TextAreaFormField name="Details" classValue="col-span-full"/>
    <div class="col-span-full">
        <h3 class="h3">Select Recievers</h3>
        <ListBox multiple> 
            {#each data.departments as department}
                <ListBoxItem bind:group={valueMultiple} name="department" value={department.id}>{department.name}</ListBoxItem>
            {/each}
        </ListBox>

    </div>
</AddForm>