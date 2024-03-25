import type { Course, Student, Attendance, CourseEnrollment } from '$lib/models';
import { courseStore, studentStore, attendanceStore, courseEnrollmentStore } from '$lib/stores';

export async function getCourses(): Promise<Course[]> {
    return new Promise((resolve) => {
        courseStore.subscribe((courses) => {
            resolve(courses);
        })();
    });
}

export async function getStudents(): Promise<Student[]> {
    return new Promise((resolve) => {
        studentStore.subscribe((students) => {
            resolve(students);
        })();
    });
}

export async function getAttendances(): Promise<Attendance[]> {
    return new Promise((resolve) => {
        attendanceStore.subscribe((attendance) => {
            resolve(attendance);
        })();
    });
}

export async function getCourseEnrollments(): Promise<CourseEnrollment[]> {
    return new Promise((resolve) => {
        courseEnrollmentStore.subscribe((courseEnrollments) => {
            resolve(courseEnrollments);
        })();
    });
}

export async function getCourseById(id: string): Promise<Course | undefined> {
    return new Promise((resolve) => {
        courseStore.subscribe((courses) => {
            resolve(courses.find((c) => c.id === id));
        })();
    });
}

export async function getAttendancesByCourseId(courseId: string): Promise<Attendance[]> {
    return new Promise((resolve) => {
        attendanceStore.subscribe((attendance) => {
            resolve(attendance.filter((a) => a.course_id === courseId));
        })();
    });
}

export async function getStudentsByAttendanceId(attendanceId: string): Promise<Student[]> {
    return new Promise((resolve) => {
        attendanceStore.subscribe((attendance) => {
            studentStore.subscribe((students) => {
                let attendanceMap = attendance.find((a) => a.id === attendanceId)?.students;
                
                if (!attendanceMap) {
                    resolve([]);
                    return;
                }

                let studentIds = Object.keys(attendanceMap);
                resolve(
                    students.filter((s) =>
                        studentIds.includes(s.id)
                    )
                );
            })();
        })();
    });
}

export async function getStudentsByCourseId(courseId: string): Promise<Student[]> {
    return new Promise((resolve) => {
        courseEnrollmentStore.subscribe((courseEnrollments) => {
            studentStore.subscribe((students) => {
                let courseEnrollmentsMap = courseEnrollments.filter((ce) => ce.course_id === courseId);
                let studentIds = courseEnrollmentsMap.map((ce) => ce.studentsIds).flat();
                resolve(
                    students.filter((s) =>
                        studentIds.includes(s.id)
                    )
                );
            })();
        })();
    });
}

export async function getAttendanceById(id: string): Promise<Attendance | undefined> {
    return new Promise((resolve) => {
        attendanceStore.subscribe((attendance) => {
            resolve(attendance.find((a) => a.id === id));
        })();
    });
}

export async function addAttendance(attendance: Attendance) {
    attendanceStore.update((attendances) => {
        return [...attendances, attendance];
    });
}