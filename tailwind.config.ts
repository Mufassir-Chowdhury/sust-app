import { join } from 'path'
import type { Config } from 'tailwindcss'
import forms from '@tailwindcss/forms';
import typography from '@tailwindcss/typography';
import { skeleton } from '@skeletonlabs/tw-plugin'

export default {
	darkMode: 'class',
	content: ['./src/**/*.{html,js,svelte,ts}', join(require.resolve('@skeletonlabs/skeleton'), '../**/*.{html,js,svelte,ts}'),
    "./node_modules/flowbite-svelte/**/*.{html,js,svelte,ts}",],
	theme: {
		extend: {},
	},
	plugins: [
		    require('flowbite/plugin'),
		forms,
		typography,
		skeleton({
			themes: {
				preset: [
					{
						name: 'gold-nouveau',
						enhancements: true,
					},
				],
			},
		}),
	],
} satisfies Config;
