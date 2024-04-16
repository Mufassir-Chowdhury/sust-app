<script lang="ts">
	import { ListBox, ListBoxItem } from '@skeletonlabs/skeleton';

    export let data;
    let courses = data.allCourses;
    let takenCourseCodes = data.registeredCourses.map(course => course.id);
    let year = "2019";
    let semester = "1";
    let valueMultiple: string[] = [];
    $: courses = data.allCourses.filter(course => String(course.year).toString() == year && String(course.semester).toString() == semester && !takenCourseCodes.includes(course.id));

    const registerCourses = async () => {
        const response = await fetch('/api/student/register', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                courses: valueMultiple,
                student: data.id,
            }),
        }).then((res) => res.json());
        window.location.reload();
    };
</script>

<div class="p-8">
    <h2 class="h2">
        Course Registration
    </h2>
    <h4 class="h4 font-semibold mt-8">
        Registered Courses:
    </h4>
    <div class="mt-4">
        {#if data.registeredCourses.length === 0}
            <p class="text-gray-500">No courses registered</p>
        {/if}
        {#each data.registeredCourses as course}
            <div class="flex justify-between items-center border-b border-gray-200 py-2">
                <div>
                    <p class="text-sm font-semibold">{course.name}</p>
                    <p class="text-xs text-gray-500">{course.id}</p>
                </div>
                <p class="text-sm font-semibold">{course.credit}</p>
            </div>
        {/each}
    </div>
    <h4 class="h4 font-semibold mt-8">
        Available Courses:
    </h4>
    <div class="mt-4">
        <div class="flex w-full justify-between my-4">
            <select name="year" id="year" bind:value={year}>
                <option value="2022">2022</option>
                <option value="2021">2021</option>
                <option value="2020">2020</option>
                <option value="2019">2019</option>
            </select>
            <select name="semester" id="semester" bind:value={semester}>
                <option value="1">1</option>
                <option value="2">2</option>
                <option value="3">3</option>
                <option value="4">4</option>
                <option value="5">5</option>
                <option value="6">6</option>
                <option value="7">7</option>
                <option value="8">8</option>
            </select>
            <button class="btn variant-filled-primary" on:click|preventDefault={registerCourses}>Save</button>
        </div>

        <ListBox multiple>
            {#each courses as course}
                <ListBoxItem bind:group={valueMultiple} name="course" value={course.id}>{course.name}</ListBoxItem>
            {/each}
        </ListBox>
			
    </div>
</div>