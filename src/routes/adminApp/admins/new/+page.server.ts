import type { Actions } from './$types';

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        
	},
} satisfies Actions;