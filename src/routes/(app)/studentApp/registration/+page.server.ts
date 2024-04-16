import { getCourseByDepartment, getRegisteredCourses } from '$lib/Database/course';
import type { PageServerLoad  } from './$types';

export const load: PageServerLoad  = async ({ locals }) => {
    const registeredCourses = await getRegisteredCourses(locals.user.id);
    const allCourses = await getCourseByDepartment(locals.user.department);
    return { registeredCourses, allCourses, id: locals.user.id };
};