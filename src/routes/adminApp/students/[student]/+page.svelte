<script>
	import DetailsPage from "$lib/DetailsPage.svelte";
    export let data;

    let photo = "https://i.pravatar.cc/";
    let name = 'Mufassir Ahmad Chowdhury';
    let details=[
        {
            'key':'ID',
            'value':'2019331073'
        },
        {
            'key': 'Department',
            'value': 'Computer Science and Engineering'
        },
        {
            'key': 'Email',
            'value': 'mufassir73@student.sust.edu'
        },
        {
            'key': 'Gender',
            'value': 'Male'
        },
        {
            'key': 'Blood Group',
            'value': 'B+'
        },
        {
            'key': 'Session',
            'value': '2019'
        },
        {
            'key': 'Current Semester',
            'value': '3'
        }
    ]
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item="Mufassir Ahmad Chowdhury"
        details={details}
        photo={photo}
        name={name}/>
