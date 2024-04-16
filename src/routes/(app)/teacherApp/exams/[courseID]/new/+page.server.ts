import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import { DateTime } from 'luxon';
import { db } from '$lib/Database/surreal';

export const actions = {
	default: async ({ request, params }) => {
        const data = await request.formData();
       
        const [record] = await db.create("exam", {
            title: data.get('title'),
            total_grade: data.get('total-grade'),
            due_date: data.get('due-date'),
            details: data.get('details'),
            creation_date: DateTime.now().toISODate(),
        });
        await db.query('RELATE $course->has->$exam', { course: params.courseID, exam: record.id });
        // TODO handle failure
        if(record){
            throw redirect(303, "/teacherApp/exams/" + params.courseID + "/" + record.id + "?showSuccess=true");
        }
        
	},
} satisfies Actions;