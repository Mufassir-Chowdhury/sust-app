<script lang="js">
    // @ts-ignore
    import Calendar from '@event-calendar/core';
    // @ts-ignore
    import TimeGrid from '@event-calendar/time-grid';

    let plugins = [TimeGrid];
    let time = new Date();
    setInterval(() => {
        time = new Date();
    }, 1000*60);
    let options = {
        view: 'timeGridWeek',
        allDaySlot: false,
        duration: {days: 3},
        slotMaxTime: '18:00:00',
        slotMinTime: '08:00:00',
        events: [
            {
                title: 'Event 1',
                start: '2023-8-30 09:00:00',
                end: '2023-8-30 10:00:00',
            },
            {
                title: 'Event 1',
                start: '2023-8-30 09:00:00',
                end: '2023-8-30 10:00:00',
            },
            
        ]
    };
</script>

<div class="grid grid-cols-10 p-8">
    <div class="col-span-4 flex flex-col">
        <div class="text-center">
            <div class=" h1 text-8xl">{time.getHours()}:{time.getMinutes()}</div>
            <div class="tracking-widest h-fit">{time.toDateString()}</div>
        </div>
        <div class="mt-24">
            <div class="text-center text-2xl">Courses</div>
            <div class="text-center text-2xl">No Upcoming Events</div>
        </div>
    </div>
    <div class="col-span-6 overflow-scroll">

        <Calendar {plugins} {options} />
    </div>
</div>