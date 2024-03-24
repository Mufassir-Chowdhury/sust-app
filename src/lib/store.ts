import { writable } from 'svelte/store';
import type { Attendance, Course, Student } from './models';

export const courseStore = writable<Course[]>([]);
export const studentStore = writable<Student[]>([]);
export const attendanceStore = writable<Attendance[]>([]);