<script lang="ts">
	import { onMount } from "svelte";
	import Swal from "sweetalert2";

  export let appUrl: String;
  export let pageName: String;
  export let url: String;
  export let appName: String;
  export let item: string;
  export let title: string;
  onMount(() => {
      if (new URL(window.location.href).searchParams.get('showSuccess') === 'true') {
          Swal.fire({
              title: "Successful!",
              text: `New ${pageName} is registered!`,
              icon: "success"
          });
      }
  });
</script>

<div class="p-8">
    <ol class="breadcrumb col-span-full mb-8">
        <li class="crumb"><a class="anchor" href="/{appUrl}">{appName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li class="crumb"><a class="anchor" href="/{appUrl}/{url}">{pageName}</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li>{item}</li>
    </ol>
    <div class="mt-6 border-t border-gray-100">
      <div class="flex flex-col items-center">
          <dd class="text-xl font-semibold leading-6 text-gray-700 mt-0 mb-8">{title}</dd>
      </div>
      <dl class="divide-y">
        <slot/>
      </dl>
    </div>
  </div>
  