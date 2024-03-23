<script lang="ts">
    let currentComment = '';
    const statusMap: Record<string, string> = {
        "green": "text-green-500",
        "red": "text-red-500",
        "yellow": "text-yellow-500",
        "black": "text-black",
    }
    export let details: any;
</script>

<div class="text-center {statusMap[details.status_color]}">
    {details.status}
</div>
<div class="px-24 py-4 border-b-2">
    <div class="font-thin">
        {details.course}   |   {details.creation_date}
    </div>
    {#if details.total_grade}
        <div class="flex justify-between font-semibold">
            <div>
                Total points: {details.total_grade}
            </div>
            <div>
                Due: {details.due_date}
            </div>
        </div>
    {/if}
</div>
<div class="grid grid-cols-3 p-4 gap-6">
    <div class="col-span-2 h-full flex flex-col justify-between">
        <div>
            {details.description}
        </div>
        <div class="input-group input-group-divider grid-cols-[auto_1fr_auto] rounded-container-token">
            <button class="input-group-shim">+</button>
            <textarea
                bind:value={currentComment}
                class="bg-transparent border-0 ring-0"
                name="prompt"
                id="prompt"
                placeholder="Write a comment..."
                rows="1"
            />
            <button class="variant-filled-primary">Send</button>
        </div>
    </div>
    <div class="flex flex-col gap-4 ">
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
        {#if details.submissions}
            
            <div class="card">
                <div class="p-4">
                    <div class="font-semibold">
                        Submissions
                    </div>
                    <div class="flex flex-col gap-4 mt-4">
                        <button class="btn variant-outline-primary">
                            Add or create
                        </button>
                        <button class="btn variant-form-material">
                            Turn in
                        </button>
                    </div>
                </div>
            </div>
        {/if}
    </div>
</div>
<div class=" p-4">
    <div class="font-semibold">
        Comments
    </div>
    <div>
        No comments yet!
    </div>
</div>