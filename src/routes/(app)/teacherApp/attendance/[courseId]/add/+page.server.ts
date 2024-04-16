import { addClass, getCourse } from "$lib/Database/course.js";
import { getStudentListByCourse } from "$lib/Database/student.js";
import type { Actions } from '@sveltejs/kit';

export const actions: Actions = {
    formAction: async ({ request }) => {
      const formData = await request.formData();
      const attendanceJson = formData.get('attendance')?.toString();
  
      if (attendanceJson) {
        try {
          const attendance = JSON.parse(attendanceJson);
          await addClass(attendance);
          return { success: true };
        } catch (error) {
          console.error('Error adding attendance:', error);
          return { success: false, error: 'Failed to add attendance' };
        }
      } else {
        return { success: false, error: 'No attendance data provided' };
      }
    },
  };
export async function load( { params } ) {
    let course = await getCourse(params.courseId);
    let students = await getStudentListByCourse(params.courseId);

    return {
        course: course,
        students: students,
    }
}