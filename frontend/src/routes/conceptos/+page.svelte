<script lang="ts">
  import { onMount } from 'svelte';

  interface Concepto {
    id?: number;
    clave_prod_serv: string;
    clave_unidad: string;
    descripcion: string;
    valor_unitario: number;
  }

  let conceptos: Concepto[] = $state([]);
  let searchQuery = $state('');
  let isEditing = $state(false);
  let showModal = $state(false);
  let currentConceptId = $state<number | null>(null);

  // Form states
  let claveProdServ = $state('');
  let claveUnidad = $state('LTR');
  let descripcion = $state('');
  let valorUnitario = $state<number | null>(null);
  let errorMessage = $state('');

  // SAT standard presets for easy creation
  const prodServPresets = [
    { code: '15101514', name: 'Gasolina regular menor a 91 octanos (Magna)' },
    { code: '15101515', name: 'Gasolina premium mayor o igual a 91 octanos (Premium)' },
    { code: '15101505', name: 'Combustible diésel' },
    { code: '15101512', name: 'Gas Licuado de Petróleo (L.P.)' },
    { code: '15121501', name: 'Aceite de motor' },
    { code: '24111802', name: 'Tanques de gas' },
    { code: '78181507', name: 'Servicio de lavado de vehículos' }
  ];

  const unidadPresets = [
    { code: 'LTR', name: 'LTR - Litro' },
    { code: 'H87', name: 'H87 - Pieza' },
    { code: 'KGM', name: 'KGM - Kilogramo' },
    { code: 'E48', name: 'E48 - Unidad de servicio' }
  ];

  const loadConceptos = async () => {
    try {
      const response = await fetch('http://localhost:8080/concepto');
      if (response.ok) {
        conceptos = await response.json();
      }
    } catch (error) {
      console.error('Error cargando conceptos:', error);
    }
  };

  onMount(loadConceptos);

  const applyPreset = (preset: { code: string, name: string }) => {
    claveProdServ = preset.code;
    if (!descripcion) {
      descripcion = preset.name;
    }
  };

  const openAddModal = () => {
    isEditing = false;
    currentConceptId = null;
    claveProdServ = '';
    claveUnidad = 'LTR';
    descripcion = '';
    valorUnitario = null;
    errorMessage = '';
    showModal = true;
  };

  const openEditModal = (con: Concepto) => {
    isEditing = true;
    currentConceptId = con.id || null;
    claveProdServ = con.clave_prod_serv;
    claveUnidad = con.clave_unidad;
    descripcion = con.descripcion;
    valorUnitario = con.valor_unitario;
    errorMessage = '';
    showModal = true;
  };

  const validateForm = () => {
    const cleanClave = claveProdServ.trim();
    if (!cleanClave) return 'La clave de producto/servicio es requerida';
    if (cleanClave.length !== 8 || isNaN(Number(cleanClave))) return 'La clave de producto/servicio debe ser de 8 dígitos del catálogo del SAT';

    const cleanUnidad = claveUnidad.trim().toUpperCase();
    if (!cleanUnidad) return 'La clave de unidad es requerida';
    if (cleanUnidad.length < 2 || cleanUnidad.length > 3) return 'La clave de unidad debe tener entre 2 y 3 caracteres (Ej: LTR)';

    if (!descripcion.trim()) return 'La descripción del concepto es requerida';
    
    if (valorUnitario === null || valorUnitario === undefined || isNaN(valorUnitario) || valorUnitario < 0) {
      return 'El valor unitario debe ser un número mayor o igual a 0';
    }

    return null;
  };

  const saveConcepto = async () => {
    const error = validateForm();
    if (error) {
      errorMessage = error;
      return;
    }

    const payload: Concepto = {
      clave_prod_serv: claveProdServ.trim(),
      clave_unidad: claveUnidad.trim().toUpperCase(),
      descripcion: descripcion.trim(),
      valor_unitario: Number(valorUnitario)
    };

    try {
      let response;
      if (isEditing && currentConceptId !== null) {
        response = await fetch(`http://localhost:8080/concepto/${currentConceptId}`, {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
      } else {
        response = await fetch('http://localhost:8080/concepto', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
      }

      if (response.ok) {
        showModal = false;
        await loadConceptos();
      } else {
        const data = await response.json();
        errorMessage = data.error || 'Error al guardar el concepto';
      }
    } catch (err) {
      console.error(err);
      errorMessage = 'Error de conexión con el servidor';
    }
  };

  const deleteConcepto = async (id: number) => {
    if (!confirm('¿Está seguro de eliminar este concepto?')) return;
    try {
      const response = await fetch(`http://localhost:8080/concepto/${id}`, {
        method: 'DELETE'
      });
      if (response.ok) {
        await loadConceptos();
      }
    } catch (error) {
      console.error('Error al borrar:', error);
    }
  };

  let filteredConceptos = $derived(
    conceptos.filter(
      (c) =>
        c.descripcion.toLowerCase().includes(searchQuery.toLowerCase()) ||
        c.clave_prod_serv.includes(searchQuery)
    )
  );

  const getUnidadName = (code: string) => {
    return unidadPresets.find((u) => u.code === code)?.name || code;
  };
</script>

<div class="container animate-fade-in">
  <header class="page-header">
    <div>
      <h1>Catálogo de Conceptos (Productos/Servicios)</h1>
      <p class="subtitle">Administra tus combustibles, aceites y otros servicios facturables con códigos SAT oficiales</p>
    </div>
    <button class="btn btn-primary" onclick={openAddModal}>
      <i class="fa-solid fa-plus"></i> Nuevo Concepto
    </button>
  </header>

  <!-- Search and Stats -->
  <div class="search-bar-container">
    <div class="search-input-wrapper">
      <i class="fa-solid fa-magnifying-glass search-icon"></i>
      <input
        type="text"
        placeholder="Buscar por descripción o clave SAT..."
        bind:value={searchQuery}
        class="search-input"
      />
    </div>
    <div class="stats-badge">
      <i class="fa-solid fa-tags"></i> {filteredConceptos.length} Concepto{filteredConceptos.length !== 1 ? 's' : ''}
    </div>
  </div>

  <!-- Cards Grid for Conceptos -->
  <div class="concept-grid">
    {#each filteredConceptos as con}
      <div class="concept-card animate-fade-in">
        <div class="card-header">
          <div class="tag-badges">
            <span class="badge clave-badge" title="Clave de Producto/Servicio SAT">
              <i class="fa-solid fa-barcode"></i> {con.clave_prod_serv}
            </span>
            <span class="badge unidad-badge" title="Clave de Unidad SAT">
              <i class="fa-solid fa-box-open"></i> {con.clave_unidad}
            </span>
          </div>
          <div class="card-actions">
            <button class="action-btn edit" onclick={() => openEditModal(con)} title="Editar">
              <i class="fa-solid fa-pen"></i>
            </button>
            <button class="action-btn delete" onclick={() => deleteConcepto(con.id!)} title="Eliminar">
              <i class="fa-solid fa-trash"></i>
            </button>
          </div>
        </div>
        <div class="card-body">
          <h3 class="concept-title">{con.descripcion}</h3>
          <p class="concept-unidad-name">{getUnidadName(con.clave_unidad)}</p>
        </div>
        <div class="card-footer">
          <div class="price-section">
            <span class="price-label">Valor Unitario</span>
            <span class="price-value">${con.valor_unitario.toFixed(4)} <span class="currency">MXN</span></span>
          </div>
        </div>
      </div>
    {:else}
      <div class="empty-state-card col-span-3">
        <i class="fa-solid fa-tag empty-icon"></i>
        <p>No se encontraron conceptos en el catálogo. ¡Crea uno nuevo!</p>
      </div>
    {/each}
  </div>
</div>

<!-- Modal Form -->
{#if showModal}
  <div class="modal-overlay animate-fade-in" onclick={() => showModal = false} role="presentation">
    <div class="modal-card animate-scale-up" onclick={(e) => e.stopPropagation()} role="presentation">
      <header class="modal-header">
        <h2>{isEditing ? 'Editar Concepto' : 'Agregar Nuevo Concepto'}</h2>
        <button class="close-btn" onclick={() => showModal = false}>&times;</button>
      </header>

      <form onsubmit={(e) => { e.preventDefault(); saveConcepto(); }}>
        <div class="modal-body">
          {#if errorMessage}
            <div class="alert alert-danger animate-shake">
              <i class="fa-solid fa-triangle-exclamation"></i> {errorMessage}
            </div>
          {/if}

          <!-- Quick presets list (only visible when adding) -->
          {#if !isEditing}
            <div class="presets-section">
              <span class="presets-title">Sugerencias Rápidas:</span>
              <div class="presets-list">
                {#each prodServPresets as preset}
                  <button type="button" class="preset-pill" onclick={() => applyPreset(preset)}>
                    {preset.code} ({preset.name.split(' ')[0]})
                  </button>
                {/each}
              </div>
            </div>
          {/if}

          <div class="form-grid">
            <div class="form-group col-span-2">
              <label for="descripcion">Descripción del Producto o Servicio</label>
              <input
                id="descripcion"
                type="text"
                bind:value={descripcion}
                placeholder="EJ: Gasolina Magna Premium 87 Octanos"
                required
                class="form-control"
              />
            </div>

            <div class="form-group">
              <label for="clave_prod">Clave Producto o Servicio (SAT 8 dígitos)</label>
              <input
                id="clave_prod"
                type="text"
                bind:value={claveProdServ}
                placeholder="Ej: 15101514"
                maxlength="8"
                required
                class="form-control"
              />
            </div>

            <div class="form-group">
              <label for="clave_uni">Clave Unidad</label>
              <select id="clave_uni" bind:value={claveUnidad} class="form-control">
                {#each unidadPresets as uni}
                  <option value={uni.code}>{uni.name}</option>
                {/each}
              </select>
            </div>

            <div class="form-group col-span-2">
              <label for="valor">Valor Unitario ($)</label>
              <input
                id="valor"
                type="number"
                step="0.0001"
                min="0"
                bind:value={valorUnitario}
                placeholder="0.0000"
                required
                class="form-control"
              />
            </div>
          </div>
        </div>

        <footer class="modal-footer">
          <button type="button" class="btn btn-secondary" onclick={() => showModal = false}>Cancelar</button>
          <button type="submit" class="btn btn-primary">
            <i class="fa-solid fa-floppy-disk"></i> Guardar Concepto
          </button>
        </footer>
      </form>
    </div>
  </div>
{/if}

<style>
  .container {
    max-width: 1200px;
    margin: 0 auto;
  }

  .page-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 28px;
  }

  .page-header h1 {
    font-size: 1.8rem;
    font-weight: 800;
    margin: 0 0 6px 0;
    color: #1a202c;
  }

  .subtitle {
    margin: 0;
    color: #718096;
    font-size: 0.95rem;
  }

  /* Search & Stats Bar */
  .search-bar-container {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    gap: 16px;
  }

  .search-input-wrapper {
    position: relative;
    flex: 1;
  }

  .search-icon {
    position: absolute;
    left: 16px;
    top: 50%;
    transform: translateY(-50%);
    color: #a0aec0;
  }

  .search-input {
    width: 100%;
    padding: 12px 16px 12px 48px;
    border-radius: 10px;
    border: 1px solid #e2e8f0;
    font-size: 0.95rem;
    transition: all 0.2s ease;
    background: #fff;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.02);
  }

  .search-input:focus {
    border-color: #4facfe;
    outline: none;
    box-shadow: 0 0 0 3px rgba(79, 172, 254, 0.15);
  }

  .stats-badge {
    background: #edf2f7;
    color: #4a5568;
    padding: 10px 16px;
    border-radius: 10px;
    font-weight: 600;
    font-size: 0.9rem;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  /* Grid Layout for Cards */
  .concept-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(340px, 1fr));
    gap: 20px;
  }

  .concept-card {
    background: #fff;
    border-radius: 14px;
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.03);
    border: 1px solid #edf2f7;
    padding: 20px;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    transition: all 0.2s ease;
    position: relative;
    overflow: hidden;
  }

  .concept-card:hover {
    transform: translateY(-2px);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.06);
    border-color: #cbd5e1;
  }

  .concept-card::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    width: 4px;
    height: 100%;
    background: linear-gradient(to bottom, #4facfe, #00f2fe);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 14px;
  }

  .tag-badges {
    display: flex;
    gap: 6px;
  }

  .badge {
    padding: 4px 8px;
    border-radius: 6px;
    font-size: 0.75rem;
    font-weight: 700;
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .clave-badge {
    background: #e0f2fe;
    color: #0369a1;
  }

  .unidad-badge {
    background: #f1f5f9;
    color: #475569;
  }

  .card-actions {
    display: flex;
    gap: 2px;
  }

  .action-btn {
    background: none;
    border: none;
    cursor: pointer;
    font-size: 0.9rem;
    padding: 6px;
    border-radius: 6px;
    transition: all 0.2s;
  }

  .action-btn.edit {
    color: #4facfe;
  }

  .action-btn.edit:hover {
    background: rgba(79, 172, 254, 0.08);
  }

  .action-btn.delete {
    color: #e53e3e;
  }

  .action-btn.delete:hover {
    background: rgba(229, 62, 62, 0.08);
  }

  .card-body {
    flex: 1;
    margin-bottom: 16px;
  }

  .concept-title {
    font-size: 1.1rem;
    font-weight: 700;
    color: #1e293b;
    margin: 0 0 4px 0;
    line-height: 1.4;
  }

  .concept-unidad-name {
    margin: 0;
    color: #64748b;
    font-size: 0.85rem;
    font-weight: 500;
  }

  .card-footer {
    border-top: 1px solid #f1f5f9;
    padding-top: 14px;
    display: flex;
    justify-content: space-between;
    align-items: flex-end;
  }

  .price-section {
    display: flex;
    flex-direction: column;
  }

  .price-label {
    font-size: 0.75rem;
    color: #94a3b8;
    text-transform: uppercase;
    font-weight: 700;
    letter-spacing: 0.5px;
  }

  .price-value {
    font-size: 1.3rem;
    font-weight: 800;
    color: #0f172a;
    letter-spacing: -0.5px;
  }

  .currency {
    font-size: 0.8rem;
    color: #64748b;
    font-weight: 600;
  }

  .empty-state-card {
    background: #fff;
    border-radius: 14px;
    border: 1px dashed #cbd5e1;
    padding: 60px;
    text-align: center;
    color: #94a3b8;
  }

  .empty-icon {
    font-size: 2.5rem;
    margin-bottom: 12px;
    color: #cbd5e0;
  }

  /* Preset Pills */
  .presets-section {
    background: #f8fafc;
    border-radius: 10px;
    padding: 14px;
    margin-bottom: 20px;
    border: 1px solid #e2e8f0;
  }

  .presets-title {
    display: block;
    font-size: 0.8rem;
    font-weight: 700;
    color: #64748b;
    margin-bottom: 8px;
    text-transform: uppercase;
  }

  .presets-list {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }

  .preset-pill {
    background: #fff;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 0.75rem;
    color: #334155;
    cursor: pointer;
    transition: all 0.2s;
    font-weight: 500;
  }

  .preset-pill:hover {
    background: #4facfe;
    color: #fff;
    border-color: #4facfe;
  }

  /* Buttons */
  .btn {
    padding: 10px 20px;
    font-size: 0.9rem;
    font-weight: 600;
    border-radius: 8px;
    border: none;
    cursor: pointer;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    transition: all 0.2s;
  }

  .btn-primary {
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    color: #fff;
    box-shadow: 0 4px 10px rgba(79, 172, 254, 0.3);
  }

  .btn-primary:hover {
    transform: translateY(-1px);
    box-shadow: 0 6px 14px rgba(79, 172, 254, 0.4);
  }

  .btn-secondary {
    background: #edf2f7;
    color: #4a5568;
  }

  .btn-secondary:hover {
    background: #e2e8f0;
  }

  /* Modal styling */
  .modal-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    background: rgba(0, 0, 0, 0.4);
    backdrop-filter: blur(4px);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 2000;
  }

  .modal-card {
    background: #fff;
    border-radius: 14px;
    width: 100%;
    max-width: 600px;
    box-shadow: 0 10px 25px rgba(0,0,0,0.15);
    overflow: hidden;
  }

  .modal-header {
    padding: 20px 24px;
    border-bottom: 1px solid #edf2f7;
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: #f8fafc;
  }

  .modal-header h2 {
    margin: 0;
    font-size: 1.3rem;
    font-weight: 700;
    color: #1a202c;
  }

  .close-btn {
    background: none;
    border: none;
    font-size: 1.8rem;
    cursor: pointer;
    color: #a0aec0;
    line-height: 1;
    transition: color 0.2s;
  }

  .close-btn:hover {
    color: #4a5568;
  }

  .modal-body {
    padding: 24px;
  }

  .modal-footer {
    padding: 16px 24px;
    border-top: 1px solid #edf2f7;
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    background: #f8fafc;
  }

  /* Form layouts */
  .form-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
  }

  .col-span-2 {
    grid-column: span 2;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .form-group label {
    font-size: 0.85rem;
    font-weight: 600;
    color: #4a5568;
  }

  .form-control {
    padding: 10px 14px;
    border-radius: 8px;
    border: 1px solid #cbd5e0;
    font-size: 0.95rem;
    transition: all 0.2s;
  }

  .form-control:focus {
    border-color: #4facfe;
    outline: none;
    box-shadow: 0 0 0 3px rgba(79, 172, 254, 0.15);
  }

  /* Alerts */
  .alert {
    padding: 12px 16px;
    border-radius: 8px;
    margin-bottom: 20px;
    font-size: 0.9rem;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .alert-danger {
    background: #fed7d7;
    color: #9b2c2c;
    border: 1px solid #feb2b2;
  }

  /* Animations */
  .animate-fade-in {
    animation: fadeIn 0.25s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  .animate-scale-up {
    animation: scaleUp 0.3s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  @keyframes scaleUp {
    from { transform: scale(0.95); opacity: 0; }
    to { transform: scale(1); opacity: 1; }
  }
</style>
