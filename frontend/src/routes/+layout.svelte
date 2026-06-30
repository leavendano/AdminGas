<script lang="ts">
	import favicon from "$lib/assets/favicon.svg";
	import { page } from "$app/stores";

	let { children } = $props();

	let isSidebarOpen = $state(true);

	const toggleSidebar = () => {
		isSidebarOpen = !isSidebarOpen;
	};
</script>

<svelte:head>
	<link rel="icon" href={favicon} />
	<link
		rel="stylesheet"
		href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css"
		integrity="sha512-DTOQO9RWCH3ppGqcWaEA1BIZOC6xxalwEsw9c2QQeAIftl+Vegovlnee1c9QX4TctnWMn13TZye+giMm8e2LwA=="
		crossorigin="anonymous"
		referrerpolicy="no-referrer"
	/>
	<style>
		/* Estilos globales y reset básicos */
		:global(body) {
			margin: 0;
			font-family:
				"Inter",
				-apple-system,
				BlinkMacSystemFont,
				"Segoe UI",
				Roboto,
				"Helvetica Neue",
				Arial,
				sans-serif;
			background-color: #f4f7f6;
			color: #333;
			overflow-x: hidden;
		}
		:global(*) {
			box-sizing: border-box;
		}
	</style>
</svelte:head>

<div class="layout">
	<!-- Menú Lateral (Sidebar) -->
	<aside class="sidebar {isSidebarOpen ? 'open' : 'closed'}">
		<div class="sidebar-header">
			<span class="icon"
				><i class="fa-solid fa-file-invoice-dollar"></i></span
			>
			<span class="text">Facturación</span>
		</div>
		<nav class="sidebar-nav">
			<a href="/" class={$page.url.pathname === "/" ? "active" : ""}>
				<span class="icon"><i class="fa-solid fa-house"></i></span>
				<span class="text">Inicio</span>
			</a>
			<a
				href="/receptores"
				class={$page.url.pathname.startsWith("/receptores")
					? "active"
					: ""}
			>
				<span class="icon"><i class="fa-solid fa-users"></i></span>
				<span class="text">Receptores</span>
			</a>
			<a
				href="/conceptos"
				class={$page.url.pathname.startsWith("/conceptos")
					? "active"
					: ""}
			>
				<span class="icon"><i class="fa-solid fa-tags"></i></span>
				<span class="text">Conceptos</span>
			</a>
			<a
				href="/emisor"
				class={$page.url.pathname.startsWith("/emisor") ? "active" : ""}
			>
				<span class="icon"><i class="fa-solid fa-building"></i></span>
				<span class="text">Emisor</span>
			</a>
			<a
				href="/facturas"
				class={$page.url.pathname.startsWith("/facturas")
					? "active"
					: ""}
			>
				<span class="icon"
					><i class="fa-solid fa-file-invoice-dollar"></i></span
				>
				<span class="text">Facturas</span>
			</a>
		</nav>
	</aside>

	<!-- Contenedor Principal -->
	<div class="main-wrapper {isSidebarOpen ? 'shifted' : 'full'}">
		<!-- Barra Superior (Topbar) -->
		<header class="topbar">
			<button
				class="hamburger"
				onclick={toggleSidebar}
				aria-label="Alternar menú"
			>
				<svg
					viewBox="0 0 24 24"
					width="24"
					height="24"
					stroke="currentColor"
					stroke-width="2"
					fill="none"
					stroke-linecap="round"
					stroke-linejoin="round"
				>
					<line x1="3" y1="12" x2="21" y2="12"></line>
					<line x1="3" y1="6" x2="21" y2="6"></line>
					<line x1="3" y1="18" x2="21" y2="18"></line>
				</svg>
			</button>
			<div class="topbar-actions">
				<div class="profile-pic" title="Perfil de Usuario">A</div>
			</div>
		</header>

		<!-- Contenido de la Página -->
		<main class="content">
			{@render children()}
		</main>
	</div>
</div>

<style>
	.layout {
		display: flex;
		min-height: 100vh;
		overflow: hidden;
	}

	.sidebar {
		position: fixed;
		top: 0;
		left: 0;
		height: 100vh;
		background: #0f172a;
		color: #cbd5e1;
		width: 260px;
		transition:
			transform 0.3s cubic-bezier(0.4, 0, 0.2, 1),
			width 0.3s cubic-bezier(0.4, 0, 0.2, 1);
		z-index: 1000;
		box-shadow: 4px 0 15px rgba(0, 0, 0, 0.15);
		display: flex;
		flex-direction: column;
		overflow-x: hidden;
	}

	.sidebar.closed {
		transform: translateX(-100%);
	}

	.sidebar-header {
		height: 70px;
		display: flex;
		align-items: center;
		padding: 0 24px;
		background: #0b0f19;
		border-bottom: 1px solid rgba(255, 255, 255, 0.05);
		transition: padding 0.3s cubic-bezier(0.4, 0, 0.2, 1);
	}

	.sidebar-header .icon {
		font-size: 1.3rem;
		margin-right: 16px;
		width: 32px;
		height: 32px;
		display: flex;
		align-items: center;
		justify-content: center;
		color: #38bdf8;
		background: rgba(56, 189, 248, 0.1);
		border-radius: 8px;
		transition: margin-right 0.3s cubic-bezier(0.4, 0, 0.2, 1);
	}

	.sidebar-header .text {
		font-weight: 700;
		font-size: 1.15rem;
		color: #f8fafc;
		letter-spacing: 0.5px;
		display: inline-block;
		transition:
			max-width 0.3s cubic-bezier(0.4, 0, 0.2, 1),
			opacity 0.3s cubic-bezier(0.4, 0, 0.2, 1);
		opacity: 1;
		max-width: 150px;
		overflow: hidden;
		white-space: nowrap;
	}

	.sidebar-nav {
		flex: 1;
		padding: 24px 0;
		overflow-y: auto;
	}

	.sidebar-nav a {
		display: flex;
		align-items: center;
		padding: 14px 24px;
		color: #94a3b8;
		text-decoration: none;
		transition:
			background-color 0.2s ease,
			color 0.2s ease,
			border-left-color 0.2s ease,
			padding 0.3s cubic-bezier(0.4, 0, 0.2, 1);
		border-left: 4px solid transparent;
		font-size: 1.05rem;
		white-space: nowrap;
	}

	.sidebar-nav a:hover {
		background: rgba(255, 255, 255, 0.03);
		color: #f1f5f9;
	}

	.sidebar-nav a.active {
		background: rgba(56, 189, 248, 0.08);
		color: #38bdf8;
		border-left-color: #38bdf8;
	}

	.sidebar-nav .icon {
		font-size: 1.3rem;
		margin-right: 16px;
		width: 28px;
		text-align: center;
		transition: margin-right 0.3s cubic-bezier(0.4, 0, 0.2, 1);
	}

	.sidebar-nav .text {
		font-weight: 500;
		display: inline-block;
		transition:
			max-width 0.3s cubic-bezier(0.4, 0, 0.2, 1),
			opacity 0.3s cubic-bezier(0.4, 0, 0.2, 1);
		opacity: 1;
		max-width: 200px;
		overflow: hidden;
		vertical-align: middle;
	}

	.main-wrapper {
		flex: 1;
		display: flex;
		flex-direction: column;
		min-height: 100vh;
		transition: margin-left 0.3s cubic-bezier(0.4, 0, 0.2, 1);
		width: 100%;
	}

	/* En escritorio, el contenido se desplaza. En móviles, el sidebar se superpone. */
	@media (min-width: 768px) {
		.sidebar.closed {
			transform: translateX(0);
			width: 80px;
		}

		.sidebar.closed .sidebar-header {
			padding: 0;
			justify-content: center;
		}

		.sidebar.closed .sidebar-header .icon {
			margin-right: 0;
		}

		.sidebar.closed .sidebar-header .text {
			max-width: 0;
			opacity: 0;
			pointer-events: none;
		}

		.sidebar.closed .sidebar-nav a {
			padding: 14px 0;
			justify-content: center;
		}

		.sidebar.closed .sidebar-nav .icon {
			margin-right: 0;
		}

		.sidebar.closed .sidebar-nav .text {
			max-width: 0;
			opacity: 0;
			pointer-events: none;
		}

		.main-wrapper.shifted {
			margin-left: 260px;
		}
		.main-wrapper.full {
			margin-left: 80px;
		}
	}

	.topbar {
		height: 70px;
		background: rgba(255, 255, 255, 0.95);
		backdrop-filter: blur(8px);
		box-shadow: 0 2px 10px rgba(0, 0, 0, 0.05);
		display: flex;
		align-items: center;
		justify-content: space-between;
		padding: 0 24px;
		position: sticky;
		top: 0;
		z-index: 900;
	}

	.hamburger {
		background: none;
		border: none;
		cursor: pointer;
		color: #4a5568;
		display: flex;
		align-items: center;
		justify-content: center;
		padding: 10px;
		border-radius: 8px;
		transition: all 0.2s ease;
	}

	.hamburger:hover {
		background: #edf2f7;
		color: #1a202c;
		transform: scale(1.05);
	}

	.hamburger:active {
		transform: scale(0.95);
	}

	.topbar-actions {
		display: flex;
		align-items: center;
	}

	.profile-pic {
		width: 40px;
		height: 40px;
		border-radius: 50%;
		background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
		color: white;
		display: flex;
		align-items: center;
		justify-content: center;
		font-weight: bold;
		font-size: 1.1rem;
		cursor: pointer;
		box-shadow: 0 3px 6px rgba(0, 0, 0, 0.15);
		transition: transform 0.2s;
	}

	.profile-pic:hover {
		transform: translateY(-2px);
		box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
	}

	.content {
		padding: 32px;
		flex: 1;
		overflow-y: auto;
	}
</style>
