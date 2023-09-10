<script lang="ts">
    import { Avatar } from '@skeletonlabs/skeleton';
</script>
<section class="p-4">
    <div class=" flex flex-row justify-between items-center">
        <Avatar src="https://something.com/invalid-image.jpg" initials="AB" />
        <div class="text-right">
            <h1>Mufassir Ahmad Chowdhury</h1>
            <p class="text-sm font-light">Student</p>
            <p class="text-sm font-light">Computer Science and Engineering</p>
            <p class="text-sm font-light">Third Year Second Semester</p>
        </div>
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