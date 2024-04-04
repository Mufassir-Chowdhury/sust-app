<script lang="ts">
    export let title: string;
</script>

<div class="flex justify-between">
    <h2 class="h2">
        {title}
    </h2>
    <slot name="command"></slot>
</div>
<slot></slot>
