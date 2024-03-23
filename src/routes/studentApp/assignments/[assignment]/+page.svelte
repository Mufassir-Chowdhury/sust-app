<script lang="ts">
	import DetailsPage from "$lib/DetailsPage.svelte";
	import Detailsitem from "$lib/Detailsitem.svelte";
	import { getID } from "$lib/utils.js";

    export let data;
    let details: any = data.details;
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item={details.title}
        title={details.title}>
    <div class="px-24 py-4 border-b-2">
        <div class="font-thin">
            {details.course}   |   {details.creation_date}
        </div>
        <div class="flex justify-between font-semibold">
            <div>
                Total points: {details.total_grade}
            </div>
            <div>
                Due: {details.due_date}
            </div>
        </div>
    </div>
    <div class="grid grid-cols-4 p-4">
        <div class="col-span-3">
            {details.description}
        </div>
        <div class="flex flex-col gap-4">
            <div class="card">
                <div class="p-4">
                    <div class="font-semibold">
                        Attachments
                    </div>
                    <div class="flex justify-between">
                        <div>
                            {#if details.attachments.length == 0}
                                No attachments
                            {:else}
                                {#each details.attachments as attachment}
                                    <div>
                                        <a href={attachment.url} target="_blank">
                                            {attachment.name}
                                        </a>
                                    </div>
                                {/each}
                            {/if}
                        </div>
                    </div>
                </div>
            </div>
            <div class="card">
                <div class="p-4">
                    <div class="font-semibold">
                        Submissions
                    </div>
                    <div class="flex justify-between">
                        <div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</DetailsPage>  
