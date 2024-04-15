<script lang="ts">
    import { Avatar } from '@skeletonlabs/skeleton';
    export let user: any;
</script>
<section class="p-4 my-4 flex flex-col gap-8">
    <div class="p-4 shadow-xl">
        <h4 class="h4">Profile</h4>
        <div class=" flex flex-row justify-between items-center ">
            <Avatar src="https://something.com/invalid-image.jpg" initials="AB" />
            <div class="text-right">
                <h1>{user.name}</h1>
                <p class="text-sm font-light">{user.id.split(':')[0][0].toUpperCase() + user.id.split(':')[0].slice(1)}</p>
                <p class="text-sm font-light">{user.departmentName}</p>
                {#if user.designation}
                    <p class="text-sm font-light">{user.designation}</p>
                {/if}
                {#if user.semester}
                    <p class="text-sm font-light">{user.semester} semester</p>
                {/if}
            </div>
        </div>
        <a href="/" >
            <div class="mt-4 flex items-center gap-4 btn variant-soft">
                <svg fill="#000000"  height="16px" width="16px" version="1.1" id="Capa_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 384.971 384.971" xml:space="preserve"><g><g id="Sign_Out"> <path d="M180.455,360.91H24.061V24.061h156.394c6.641,0,12.03-5.39,12.03-12.03s-5.39-12.03-12.03-12.03H12.03 C5.39,0.001,0,5.39,0,12.031V372.94c0,6.641,5.39,12.03,12.03,12.03h168.424c6.641,0,12.03-5.39,12.03-12.03 C192.485,366.299,187.095,360.91,180.455,360.91z"/> <path d="M381.481,184.088l-83.009-84.2c-4.704-4.752-12.319-4.74-17.011,0c-4.704,4.74-4.704,12.439,0,17.179l62.558,63.46H96.279 c-6.641,0-12.03,5.438-12.03,12.151c0,6.713,5.39,12.151,12.03,12.151h247.74l-62.558,63.46c-4.704,4.752-4.704,12.439,0,17.179 c4.704,4.752,12.319,4.752,17.011,0l82.997-84.2C386.113,196.588,386.161,188.756,381.481,184.088z"/></g><g></g><g></g><g></g><g></g><g></g><g></g></g></svg>							
                <div>
                    Logout
                </div>
            </div>
        </a>

    </div>
    <div class="flex flex-row justify-between">
        <div class="card variant-soft-primary">
            <header class="card-header">
                Due Exams
            </header>
            <div class="text-center">
                <div class="p-4">
                    <div class="flex items-center p-4 mb-4 text-sm text-yellow-800 bg-yellow-50 dark:bg-gray-800 dark:text-yellow-300" role="alert">
                        <svg class="flex-shrink-0 inline w-4 h-4 mr-3" aria-hidden="true" xmlns="http://www.w3.org/2000/svg" fill="currentColor" viewBox="0 0 20 20">
                          <path d="M10 .5a9.5 9.5 0 1 0 9.5 9.5A9.51 9.51 0 0 0 10 .5ZM9.5 4a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3ZM12 15H8a1 1 0 0 1 0-2h1v-3H8a1 1 0 0 1 0-2h2a1 1 0 0 1 1 1v4h1a1 1 0 0 1 0 2Z"/>
                        </svg>
                        <span class="sr-only">Info</span>
                        <div>
                          <span class="font-medium">Warning alert!</span> Change a few things up and try submitting again.
                        </div>
                      </div>
                </div>
            </div>
        </div>
    </div>
</section>