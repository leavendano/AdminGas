<script lang="ts">
  import { onMount } from 'svelte';

  interface Receptor {
    id?: number;
    rfc: string;
    nombre: string;
    domicilio_fiscal_receptor: string;
    regimen_fiscal_receptor: string;
    uso_cfdi: string;
  }

  let receptors: Receptor[] = $state([]);
  let searchQuery = $state('');
  let isEditing = $state(false);
  let showModal = $state(false);
  let currentReceptorId = $state<number | null>(null);

  // Form states
  let rfc = $state('');
  let nombre = $state('');
  let domicilioFiscalReceptor = $state('');
  let regimenFiscalReceptor = $state('601');
  let usoCfdi = $state('G03');
  let errorMessage = $state('');

  // SAT Catalogs
  const regimenes = [
    { code: '601', desc: '601 - General de Ley Personas Morales' },
    { code: '603', desc: '603 - Personas Morales con Fines no Lucrativos' },
    { code: '605', desc: '605 - Sueldos y Salarios e Ingresos Asimilados a Salarios' },
    { code: '606', desc: '606 - Arrendamiento' },
    { code: '608', desc: '608 - Demás ingresos' },
    { code: '612', desc: '612 - Actividades Empresariales y Profesionales' },
    { code: '621', desc: '621 - Incorporación Fiscal' },
    { code: '625', desc: '625 - Actividades Agrícolas, Ganaderas, Silvícolas y Pesqueras' },
    { code: '626', desc: '626 - Régimen Simplificado de Confianza (RESICO)' }
  ];

  const usosCfdi = [
    { code: 'G01', desc: 'G01 - Adquisición de mercancías' },
    { code: 'G02', desc: 'G02 - Devoluciones, descuentos o bonificaciones' },
    { code: 'G03', desc: 'G03 - Gastos en general' },
    { code: 'I01', desc: 'I01 - Construcciones' },
    { code: 'I02', desc: 'I02 - Mobiliario y equipo de oficina por inversiones' },
    { code: 'I03', desc: 'I03 - Equipo de transporte por inversiones' },
    { code: 'I04', desc: 'I04 - Equipo de cómputo y accesorios por inversiones' },
    { code: 'I08', desc: 'I08 - Otra maquinaria y equipo por inversiones' },
    { code: 'D01', desc: 'D01 - Honorarios médicos, dentales y gastos hospitalarios' },
    { code: 'D02', desc: 'D02 - Gastos médicos por incapacidad o discapacidad' },
    { code: 'D10', desc: 'D10 - Premios por seguros de gastos médicos' },
    { code: 'CP01', desc: 'CP01 - Pagos' },
    { code: 'CN01', desc: 'CN01 - Nómina' },
    { code: 'S01', desc: 'S01 - Sin efectos fiscales' }
  ];

  const loadReceptors = async () => {
    try {
      const response = await fetch('http://localhost:8080/receptor');
      if (response.ok) {
        receptors = await response.json();
      }
    } catch (error) {
      console.error('Error cargando receptores:', error);
    }
  };

  onMount(loadReceptors);

  const openAddModal = () => {
    isEditing = false;
    currentReceptorId = null;
    rfc = '';
    nombre = '';
    domicilioFiscalReceptor = '';
    regimenFiscalReceptor = '601';
    usoCfdi = 'G03';
    errorMessage = '';
    showModal = true;
  };

  const openEditModal = (rec: Receptor) => {
    isEditing = true;
    currentReceptorId = rec.id || null;
    rfc = rec.rfc;
    nombre = rec.nombre;
    domicilioFiscalReceptor = rec.domicilio_fiscal_receptor;
    regimenFiscalReceptor = rec.regimen_fiscal_receptor;
    usoCfdi = rec.uso_cfdi;
    errorMessage = '';
    showModal = true;
  };

  const validateForm = () => {
    // Basic CFDI 4.0 validation
    const cleanRfc = rfc.trim().toUpperCase();
    if (!cleanRfc) return 'El RFC es requerido';
    if (cleanRfc.length < 12 || cleanRfc.length > 13) return 'El RFC debe tener 12 (moral) o 13 (física) caracteres';
    
    // RFC regex check
    const rfcRegex = /^[A-Z&Ñ]{3,4}[0-9]{6}[A-Z0-9]{3}$/;
    if (!rfcRegex.test(cleanRfc)) return 'Formato de RFC inválido (Ej: VECJ880326XXX o GACM750824YYY)';

    if (!nombre.trim()) return 'El Nombre o Razón Social es requerido';
    
    const cleanCp = domicilioFiscalReceptor.trim();
    if (!cleanCp) return 'El Domicilio Fiscal (Código Postal) es requerido';
    if (cleanCp.length !== 5 || isNaN(Number(cleanCp))) return 'El Domicilio Fiscal debe ser un Código Postal de 5 dígitos';

    return null;
  };

  const saveReceptor = async () => {
    const error = validateForm();
    if (error) {
      errorMessage = error;
      return;
    }

    const payload: Receptor = {
      rfc: rfc.trim().toUpperCase(),
      nombre: nombre.trim().toUpperCase(), // SAT CFDI 4.0 matching is strictly uppercase without regimes like SA de CV (ideally)
      domicilio_fiscal_receptor: domicilioFiscalReceptor.trim(),
      regimen_fiscal_receptor: regimenFiscalReceptor,
      uso_cfdi: usoCfdi
    };

    try {
      let response;
      if (isEditing && currentReceptorId !== null) {
        response = await fetch(`http://localhost:8080/receptor/${currentReceptorId}`, {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
      } else {
        response = await fetch('http://localhost:8080/receptor', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
      }

      if (response.ok) {
        showModal = false;
        await loadReceptors();
      } else {
        const data = await response.json();
        errorMessage = data.error || 'Error al guardar el receptor';
      }
    } catch (err) {
      console.error(err);
      errorMessage = 'Error de conexión con el servidor';
    }
  };

  const deleteReceptor = async (id: number) => {
    if (!confirm('¿Está seguro de eliminar este receptor?')) return;
    try {
      const response = await fetch(`http://localhost:8080/receptor/${id}`, {
        method: 'DELETE'
      });
      if (response.ok) {
        await loadReceptors();
      }
    } catch (error) {
      console.error('Error al borrar:', error);
    }
  };

  // Filtered computed list
  let filteredReceptors = $derived(
    receptors.filter(
      (r) =>
        r.nombre.toLowerCase().includes(searchQuery.toLowerCase()) ||
        r.rfc.toLowerCase().includes(searchQuery.toLowerCase())
    )
  );

  const getRegimenDesc = (code: string) => {
    return regimenes.find((rg) => rg.code === code)?.desc || code;
  };

  const getUsoDesc = (code: string) => {
    return usosCfdi.find((u) => u.code === code)?.desc || code;
  };
</script>

<div class="container animate-fade-in">
  <header class="page-header">
    <div>
      <h1>Catálogo de Receptores (Clientes)</h1>
      <p class="subtitle">Administra los clientes y su información fiscal obligatoria para CFDI 4.0</p>
    </div>
    <button class="btn btn-primary" onclick={openAddModal}>
      <i class="fa-solid fa-plus"></i> Nuevo Receptor
    </button>
  </header>

  <!-- Search and Stats -->
  <div class="search-bar-container">
    <div class="search-input-wrapper">
      <i class="fa-solid fa-magnifying-glass search-icon"></i>
      <input
        type="text"
        placeholder="Buscar por nombre o RFC..."
        bind:value={searchQuery}
        class="search-input"
      />
    </div>
    <div class="stats-badge">
      <i class="fa-solid fa-address-book"></i> {filteredReceptors.length} Registrado{filteredReceptors.length !== 1 ? 's' : ''}
    </div>
  </div>

  <!-- Table View -->
  <div class="table-card">
    <div class="table-responsive">
      <table>
        <thead>
          <tr>
            <th>Razón Social</th>
            <th>RFC</th>
            <th>C.P. (Domicilio)</th>
            <th>Régimen Fiscal</th>
            <th>Uso de CFDI</th>
            <th class="actions-column">Acciones</th>
          </tr>
        </thead>
        <tbody>
          {#each filteredReceptors as rec}
            <tr class="table-row">
              <td class="td-nombre">
                <span class="avatar">{rec.nombre.substring(0,2)}</span>
                <div class="nombre-details">
                  <span class="main-name">{rec.nombre}</span>
                </div>
              </td>
              <td><span class="badge rfc-badge">{rec.rfc}</span></td>
              <td><span class="badge cp-badge"><i class="fa-solid fa-location-dot"></i> {rec.domicilio_fiscal_receptor}</span></td>
              <td class="text-secondary">{getRegimenDesc(rec.regimen_fiscal_receptor)}</td>
              <td class="text-secondary">{getUsoDesc(rec.uso_cfdi)}</td>
              <td class="actions-column">
                <button class="action-btn edit" onclick={() => openEditModal(rec)} title="Editar">
                  <i class="fa-solid fa-pen"></i>
                </button>
                <button class="action-btn delete" onclick={() => deleteReceptor(rec.id!)} title="Eliminar">
                  <i class="fa-solid fa-trash"></i>
                </button>
              </td>
            </tr>
          {:else}
            <tr>
              <td colspan="6" class="empty-state">
                <i class="fa-solid fa-inbox empty-icon"></i>
                <p>No se encontraron receptores que coincidan con la búsqueda.</p>
              </td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- Modal Form -->
{#if showModal}
  <div class="modal-overlay animate-fade-in" onclick={() => showModal = false} role="presentation">
    <div class="modal-card animate-scale-up" onclick={(e) => e.stopPropagation()} role="presentation">
      <header class="modal-header">
        <h2>{isEditing ? 'Editar Receptor' : 'Agregar Nuevo Receptor'}</h2>
        <button class="close-btn" onclick={() => showModal = false}>&times;</button>
      </header>

      <form onsubmit={(e) => { e.preventDefault(); saveReceptor(); }}>
        <div class="modal-body">
          {#if errorMessage}
            <div class="alert alert-danger animate-shake">
              <i class="fa-solid fa-triangle-exclamation"></i> {errorMessage}
            </div>
          {/if}

          <div class="form-grid">
            <div class="form-group col-span-2">
              <label for="nombre">Nombre o Razón Social (Exacto como Constancia de Situación Fiscal)</label>
              <input
                id="nombre"
                type="text"
                bind:value={nombre}
                placeholder="EJ: INGENIERIA Y CONSTRUCCIONES"
                required
                class="form-control"
              />
            </div>

            <div class="form-group">
              <label for="rfc">RFC</label>
              <input
                id="rfc"
                type="text"
                bind:value={rfc}
                placeholder="RFC de 12 o 13 caracteres"
                maxlength="13"
                required
                class="form-control uppercase"
              />
            </div>

            <div class="form-group">
              <label for="domicilio">Domicilio Fiscal (Código Postal)</label>
              <input
                id="domicilio"
                type="text"
                bind:value={domicilioFiscalReceptor}
                placeholder="Código Postal (5 dígitos)"
                maxlength="5"
                required
                class="form-control"
              />
            </div>

            <div class="form-group col-span-2">
              <label for="regimen">Régimen Fiscal Receptor</label>
              <select id="regimen" bind:value={regimenFiscalReceptor} class="form-control">
                {#each regimenes as reg}
                  <option value={reg.code}>{reg.desc}</option>
                {/each}
              </select>
            </div>

            <div class="form-group col-span-2">
              <label for="uso">Uso de CFDI</label>
              <select id="uso" bind:value={usoCfdi} class="form-control">
                {#each usosCfdi as u}
                  <option value={u.code}>{u.desc}</option>
                {/each}
              </select>
            </div>
          </div>
        </div>

        <footer class="modal-footer">
          <button type="button" class="btn btn-secondary" onclick={() => showModal = false}>Cancelar</button>
          <button type="submit" class="btn btn-primary">
            <i class="fa-solid fa-floppy-disk"></i> Guardar Receptor
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
    margin-bottom: 20px;
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

  /* Table design */
  .table-card {
    background: #fff;
    border-radius: 14px;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.04);
    border: 1px solid #edf2f7;
    overflow: hidden;
  }

  .table-responsive {
    overflow-x: auto;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    text-align: left;
  }

  thead {
    background: #f8fafc;
    border-bottom: 1px solid #edf2f7;
  }

  th {
    padding: 16px 24px;
    font-size: 0.85rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #718096;
  }

  td {
    padding: 16px 24px;
    font-size: 0.95rem;
    color: #2d3748;
    border-bottom: 1px solid #f1f5f9;
    vertical-align: middle;
  }

  .table-row {
    transition: background-color 0.2s;
  }

  .table-row:hover {
    background-color: #f8fafc;
  }

  .td-nombre {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .avatar {
    width: 38px;
    height: 38px;
    border-radius: 50%;
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    color: #fff;
    display: flex;
    align-items: center;
    justify-content: center;
    font-weight: 700;
    font-size: 0.85rem;
    letter-spacing: 0.5px;
  }

  .main-name {
    font-weight: 600;
    color: #1a202c;
  }

  .badge {
    display: inline-block;
    padding: 4px 8px;
    border-radius: 6px;
    font-size: 0.8rem;
    font-weight: 700;
  }

  .rfc-badge {
    background: #e0f2fe;
    color: #0369a1;
    font-family: monospace;
    font-size: 0.85rem;
  }

  .cp-badge {
    background: #fef3c7;
    color: #d97706;
  }

  .text-secondary {
    color: #718096;
    font-size: 0.9rem;
  }

  .empty-state {
    text-align: center;
    padding: 48px !important;
    color: #a0aec0;
  }

  .empty-icon {
    font-size: 2.5rem;
    margin-bottom: 12px;
    color: #cbd5e0;
  }

  /* Actions */
  .actions-column {
    text-align: right;
    width: 120px;
  }

  .action-btn {
    background: none;
    border: none;
    cursor: pointer;
    font-size: 1rem;
    padding: 8px;
    border-radius: 6px;
    transition: all 0.2s;
    margin-left: 4px;
  }

  .action-btn.edit {
    color: #4facfe;
  }

  .action-btn.edit:hover {
    background: rgba(79, 172, 254, 0.1);
  }

  .action-btn.delete {
    color: #e53e3e;
  }

  .action-btn.delete:hover {
    background: rgba(229, 62, 62, 0.1);
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
    max-width: 650px;
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

  .form-control.uppercase {
    text-transform: uppercase;
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
