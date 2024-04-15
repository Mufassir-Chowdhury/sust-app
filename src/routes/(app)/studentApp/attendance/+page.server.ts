import { getCourseEnrollments, getStudents, getCourseById, getAttendancesByCourseId } from "$lib/Database/sajiddb";

export async function load() {
    const students = await getStudents();
    if (students.length === 0) {
        return { enrolledCourses: [] };
    }

    const studentId = students[0].id;
    let enrolledCourses = await getCourseEnrollments();
    const coursesWithAttendance: any = [];

    enrolledCourses = enrolledCourses
            .sort((a, b) => {
                const [semesterA, yearA] = a.semester.split("/");
                const [semesterB, yearB] = b.semester.split("/");
                const yearDiff = parseInt(yearB) - parseInt(yearA);
                if (yearDiff !== 0) {
                    return yearDiff;
                }
                return parseInt(semesterB) - parseInt(semesterA);
            })
            .reverse();

        for (const enrollment of enrolledCourses) {
            const course = await getCourseById(enrollment.course_id);
            const attendances = await getAttendancesByCourseId(enrollment.course_id);
            const presentCount = attendances.reduce((count, attendance) => {
                const presentStudents = Object.values(attendance.students).filter(Boolean).length;
                return count + presentStudents;
            }, 0);
            const absentCount = attendances.length * enrollment.studentsIds.length - presentCount;
            coursesWithAttendance.push({
                id: enrollment.course_id,
                name: course?.name,
                code: course?.course_code,
                semester: enrollment.semester,
                presentCount: presentCount,
                absentCount: absentCount,
            });
        }
    console.log(coursesWithAttendance);

    return {
        enrolledCourses,
        coursesWithAttendance
    };
}