<script lang="ts">
	import { routes } from './routes';
	import { onMount } from "svelte";
	import Swal from "sweetalert2";

  export let appUrl: string;
  export let url: string;
  export let item: string;
  export let title: string;
  onMount(() => {
      if (new URL(window.location.href).searchParams.get('showSuccess') === 'true') {
          Swal.fire({
              title: "Successful!",
              text: `New ${routes[url].title} is registered!`,
              icon: "success"
          });
      }
  });
</script>

<div class="mt-6 border-t border-gray-100">
  <div class="flex flex-col items-center">
      <dd class="text-xl font-semibold leading-6 text-gray-700 mt-0 mb-8">{title}</dd>
  </div>
  <dl class="divide-y">
    <slot/>
  </dl>
</div>
