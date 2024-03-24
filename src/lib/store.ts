import { writable } from 'svelte/store';
import type { Attendance, Course, Student, CourseEnrollment } from './models';
import { mockCourses, mockStudents, mockAttendance, mockCourseEnrollments } from './mock-data';

export const courseStore = writable<Course[]>([]);
export const studentStore = writable<Student[]>([]);
export const attendanceStore = writable<Attendance[]>([]);
export const courseEnrollmentStore = writable<CourseEnrollment[]>([]);

courseStore.set(mockCourses);
studentStore.set(mockStudents);
attendanceStore.set(mockAttendance);
courseEnrollmentStore.set(mockCourseEnrollments);