<script>
    const links = [
        {
            name: "SUST",
            url: "https://www.sust.edu/",
        },
        {
            name: "E-Payments",
            url: "https://epayment.sust.edu/",
        },
        {
            name: "E-Services",
            url: "https://eservice.sust.edu.bd/",
        },
        {
            name: "Course Registration",
            url: "https://services.student.sust.edu:9090/",
        },
        {
            name: "SUST Admission",
            url: "https://admission.sust.edu.bd/",
        },
    ];
</script>

<div class="p-8">
    <h2 class="h2">
        Important Links
    </h2>
    <div class="grid grid-cols-3 mt-8 gap-8">
        {#each links as link}
            <a href={link.url}>
                <div class="card h-24  w-52 flex justify-center items-center">
                    {link.name}
                </div>
            </a>
        {/each}
    </div>
</div>