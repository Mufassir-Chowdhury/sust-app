<script>
	import DetailsPage from "$lib/DetailsPage.svelte";
    export let data;
    let details=[
        {
            'key':'Name',
            'value':'Intro to Computer Science'
        },
        {
            'key': 'ID',
            'value': 'CSE101'
        },
        {
            'key': 'Credit',
            'value': '3'
        },
        {
            'key': 'Department',
            'value': 'Computer Science and Engineering'
        },
        {
            'key': 'Type',
            'value': 'Theory'
        }
    ]
</script>
<DetailsPage 
        appName={data.appName} 
        pageName={data.pageName} 
        url={data.url}
        appUrl={data.appUrl}
        item="CSE101"
        details={details}/>
