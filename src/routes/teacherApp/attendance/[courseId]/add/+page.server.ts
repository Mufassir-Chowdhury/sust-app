import { getCourseById, getStudentsByCourseId, addAttendance } from "$lib/Database/sajiddb";
import type { Actions, RequestEvent } from '@sveltejs/kit';

export const actions: Actions = {
    formAction: async ({ request }) => {
      const formData = await request.formData();
      const attendanceJson = formData.get('attendance')?.toString();
  
      if (attendanceJson) {
        try {
          const attendance = JSON.parse(attendanceJson);
          await addAttendance(attendance);
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
    let course = await getCourseById(params.courseId);
    let students = await getStudentsByCourseId(params.courseId);

    return {
        course: course,
        students: students,
    }
}