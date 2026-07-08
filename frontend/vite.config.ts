import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';

export default defineConfig({
	plugins: [sveltekit()],
	server: {
		proxy: {
			'/login': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/callback': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/logout': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/auth': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/signout-callback-oidc': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/todo': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/receptor': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/emisor': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/concepto': { target: 'https://localhost:8080', changeOrigin: true, secure: false },
			'/factura': { target: 'https://localhost:8080', changeOrigin: true, secure: false }
		}
	}
});
