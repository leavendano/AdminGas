<script lang="ts">
  import { onMount } from 'svelte';

  interface ConceptoItem {
    id?: number;
    clave_prod_serv: string;
    clave_unidad: string;
    descripcion: string;
    valor_unitario: number;
    cantidad: number;
    iva_porcentaje: number; // 0, 8, 16
    subtotal: number;
    impuesto: number;
    total: number;
  }

  interface Emisor {
    id?: number;
    rfc: string;
    nombre: string;
    regimen_fiscal: string;
  }

  interface Receptor {
    id?: number;
    rfc: string;
    nombre: string;
    domicilio_fiscal_receptor: string;
    regimen_fiscal_receptor: string;
    uso_cfdi: string;
  }

  interface Factura {
    id?: number;
    emisor_id?: number | null;
    emisor_rfc: string;
    emisor_nombre: string;
    emisor_regimen_fiscal: string;
    receptor_id?: number | null;
    receptor_rfc: string;
    receptor_nombre: string;
    receptor_domicilio_fiscal: string;
    receptor_regimen_fiscal: string;
    receptor_uso_cfdi: string;
    fecha: string; // YYYY-MM-DD
    subtotal: number;
    impuestos: number;
    total: number;
    conceptos: ConceptoItem[] | string; // parsed array or string depending on state
    xml_timbrado?: string | null;
    uuid?: string | null;
    sello?: string | null;
    status?: string | null;
  }

  interface PredefinedConcepto {
    id?: number;
    clave_prod_serv: string;
    clave_unidad: string;
    descripcion: string;
    valor_unitario: number;
  }

  // Lists
  let facturas: Factura[] = $state([]);
  let receptores: Receptor[] = $state([]);
  let predefinedConceptos: PredefinedConcepto[] = $state([]);
  let activeEmisor = $state<Emisor | null>(null);

  // UI States
  let showCaptureModal = $state(false);
  let showDetailModal = $state(false);
  let selectedFactura = $state<Factura | null>(null);
  let searchQuery = $state('');
  let emisorLoading = $state(true);
  let stampingIds = $state<Record<number, boolean>>({});

  // Form states (Captura)
  let selectedReceptorId = $state<string>('');
  let selectedReceptor = $derived(receptores.find((r: Receptor) => r.id?.toString() === selectedReceptorId) || null);
  
  // Date state: defaults to today
  let invoiceDate = $state(new Date().toISOString().split('T')[0]);
  let dateWarning = $derived.by(() => {
    if (!invoiceDate) return '';
    const selected = new Date(invoiceDate + 'T12:00:00'); // Midday to avoid timezone offset
    const today = new Date();
    const diffTime = Math.abs(today.getTime() - selected.getTime());
    const diffDays = diffTime / (1000 * 60 * 60 * 24);
    if (diffDays > 3) {
      return 'Advertencia: La fecha tiene una diferencia mayor a 72 horas con el día actual, el PAC podría rechazar el CFDI.';
    }
    return '';
  });

  // Concepts builder in form
  let currentConceptos = $state<ConceptoItem[]>([]);
  
  // New line item inputs
  let selectedPredefinedId = $state<string>('');
  let lineCantidad = $state<number>(1);
  let lineDescripcion = $state<string>('');
  let lineValorUnitario = $state<number>(0);
  let lineIvaPorcentaje = $state<number>(16); // 16% default
  let lineClaveProd = $state<string>('');
  let lineClaveUnidad = $state<string>('');

  let errorMessage = $state('');
  let successMessage = $state('');

  // SAT Regimenes & Usos descriptors helper
  const regimenes = [
    { code: "601", desc: "601 - General de Ley Personas Morales" },
    { code: "603", desc: "603 - Personas Morales con Fines no Lucrativos" },
    { code: "605", desc: "605 - Sueldos y Salarios e Ingresos Asimilados a Salarios" },
    { code: "606", desc: "606 - Arrendamiento" },
    { code: "612", desc: "612 - Actividades Empresariales y Profesionales" },
    { code: "621", desc: "621 - Incorporación Fiscal" },
    { code: "626", desc: "626 - Régimen Simplificado de Confianza (RESICO)" }
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
    { code: 'CP01', desc: 'CP01 - Pagos' },
    { code: 'S01', desc: 'S01 - Sin efectos fiscales' }
  ];

  // API Loaders
  const loadEmisor = async () => {
    emisorLoading = true;
    try {
      const response = await fetch("http://localhost:8080/emisor");
      if (response.ok) {
        const list: Emisor[] = await response.json();
        if (list.length > 0) {
          activeEmisor = list[0];
        } else {
          activeEmisor = null;
        }
      }
    } catch (error) {
      console.error("Error cargando emisor:", error);
    } finally {
      emisorLoading = false;
    }
  };

  const loadReceptors = async () => {
    try {
      const response = await fetch("http://localhost:8080/receptor");
      if (response.ok) {
        receptores = await response.json();
      }
    } catch (error) {
      console.error("Error cargando receptores:", error);
    }
  };

  const loadPredefinedConceptos = async () => {
    try {
      const response = await fetch("http://localhost:8080/concepto");
      if (response.ok) {
        predefinedConceptos = await response.json();
      }
    } catch (error) {
      console.error("Error cargando catalogo conceptos:", error);
    }
  };

  const loadFacturas = async () => {
    try {
      const response = await fetch("http://localhost:8080/factura");
      if (response.ok) {
        const data: Factura[] = await response.json();
        // Parse "conceptos" JSON strings if returned as string from API
        facturas = data.map(f => {
          let parsedConceptos: ConceptoItem[] = [];
          if (typeof f.conceptos === 'string') {
            try {
              parsedConceptos = JSON.parse(f.conceptos);
            } catch (e) {
              console.error("Error parsing conceptos for factura:", f.id, e);
            }
          } else if (Array.isArray(f.conceptos)) {
            parsedConceptos = f.conceptos;
          }
          return {
            ...f,
            conceptos: parsedConceptos
          };
        });
      }
    } catch (error) {
      console.error("Error cargando facturas:", error);
    }
  };

  // Lifecycle
  onMount(async () => {
    await Promise.all([
      loadEmisor(),
      loadReceptors(),
      loadPredefinedConceptos(),
      loadFacturas()
    ]);
  });

  // Watch for predefined concept drop-down changes to auto-populate line fields
  $effect(() => {
    if (selectedPredefinedId) {
      const matched = predefinedConceptos.find(c => c.id?.toString() === selectedPredefinedId);
      if (matched) {
        lineDescripcion = matched.descripcion;
        lineValorUnitario = matched.valor_unitario;
        lineClaveProd = matched.clave_prod_serv;
        lineClaveUnidad = matched.clave_unidad;
      }
    } else {
      lineDescripcion = '';
      lineValorUnitario = 0;
      lineClaveProd = '';
      lineClaveUnidad = '';
    }
  });

  // Live total calculations for the concepts captured in current invoice
  let calculatedSubtotal = $derived(
    currentConceptos.reduce((sum, item) => sum + item.subtotal, 0)
  );
  let calculatedImpuestos = $derived(
    currentConceptos.reduce((sum, item) => sum + item.impuesto, 0)
  );
  let calculatedTotal = $derived(
    calculatedSubtotal + calculatedImpuestos
  );

  // Line item computations
  let computedLineSubtotal = $derived(lineValorUnitario * lineCantidad);
  let computedLineImpuesto = $derived(computedLineSubtotal * (lineIvaPorcentaje / 100));
  let computedLineTotal = $derived(computedLineSubtotal + computedLineImpuesto);

  // Actions
  const addConceptLine = () => {
    if (!lineDescripcion.trim()) {
      alert("La descripción del concepto es obligatoria.");
      return;
    }
    if (lineCantidad <= 0) {
      alert("La cantidad debe ser mayor a cero.");
      return;
    }
    if (lineValorUnitario < 0) {
      alert("El valor unitario no puede ser negativo.");
      return;
    }
    if (!lineClaveProd || lineClaveProd.length !== 8) {
      alert("La clave SAT del producto debe ser de 8 dígitos.");
      return;
    }

    const newLine: ConceptoItem = {
      clave_prod_serv: lineClaveProd,
      clave_unidad: lineClaveUnidad || 'LTR',
      descripcion: lineDescripcion.trim(),
      valor_unitario: lineValorUnitario,
      cantidad: lineCantidad,
      iva_porcentaje: lineIvaPorcentaje,
      subtotal: computedLineSubtotal,
      impuesto: computedLineImpuesto,
      total: computedLineTotal
    };

    currentConceptos = [...currentConceptos, newLine];

    // Reset line state
    selectedPredefinedId = '';
    lineCantidad = 1;
    lineDescripcion = '';
    lineValorUnitario = 0;
    lineIvaPorcentaje = 16;
    lineClaveProd = '';
    lineClaveUnidad = '';
  };

  const removeConceptLine = (index: number) => {
    currentConceptos = currentConceptos.filter((_, i) => i !== index);
  };

  const openCapture = () => {
    if (!activeEmisor) {
      alert("Debe configurar los datos del emisor antes de elaborar facturas.");
      return;
    }
    errorMessage = '';
    successMessage = '';
    selectedReceptorId = '';
    invoiceDate = new Date().toISOString().split('T')[0];
    currentConceptos = [];
    showCaptureModal = true;
  };

  const saveFactura = async () => {
    errorMessage = '';
    successMessage = '';

    if (!activeEmisor) {
      errorMessage = "No hay datos de emisor configurados.";
      return;
    }
    if (!selectedReceptor) {
      errorMessage = "Debe seleccionar un receptor (cliente).";
      return;
    }
    if (currentConceptos.length === 0) {
      errorMessage = "Debe agregar al menos un concepto a la factura.";
      return;
    }

    const payload: Factura = {
      emisor_id: activeEmisor.id,
      emisor_rfc: activeEmisor.rfc,
      emisor_nombre: activeEmisor.nombre,
      emisor_regimen_fiscal: activeEmisor.regimen_fiscal,
      
      receptor_id: selectedReceptor.id,
      receptor_rfc: selectedReceptor.rfc,
      receptor_nombre: selectedReceptor.nombre,
      receptor_domicilio_fiscal: selectedReceptor.domicilio_fiscal_receptor,
      receptor_regimen_fiscal: selectedReceptor.regimen_fiscal_receptor,
      receptor_uso_cfdi: selectedReceptor.uso_cfdi,
      
      fecha: invoiceDate,
      subtotal: calculatedSubtotal,
      impuestos: calculatedImpuestos,
      total: calculatedTotal,
      conceptos: currentConceptos // controller will stringify this array
    };

    try {
      const response = await fetch("http://localhost:8080/factura", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });

      if (response.ok) {
        successMessage = "Factura emitida y guardada con éxito.";
        showCaptureModal = false;
        await loadFacturas();
      } else {
        const errData = await response.json();
        errorMessage = errData.error || "Error al emitir la factura.";
      }
    } catch (e) {
      console.error(e);
      errorMessage = "Error de comunicación con el servidor.";
    }
  };

  const deleteFactura = async (id: number) => {
    if (!confirm("¿Está seguro de eliminar esta factura? (Se eliminará permanentemente de la base de datos)")) return;
    try {
      const response = await fetch(`http://localhost:8080/factura/${id}`, {
        method: 'DELETE'
      });
      if (response.ok) {
        await loadFacturas();
      } else {
        alert("Error al intentar borrar la factura.");
      }
    } catch (e) {
      console.error(e);
    }
  };

  const stampFactura = async (id: number) => {
    errorMessage = '';
    successMessage = '';
    stampingIds[id] = true;
    try {
      const response = await fetch(`http://localhost:8080/factura/${id}/timbrar`, {
        method: 'POST'
      });
      const data = await response.json();
      if (response.ok) {
        successMessage = `Factura timbrada con éxito. UUID: ${data.uuid}`;
        await loadFacturas();
      } else {
        errorMessage = data.error || "Ocurrió un error al timbrar la factura.";
        alert(errorMessage);
      }
    } catch (e) {
      console.error(e);
      errorMessage = "Error de comunicación con el servidor al intentar timbrar.";
      alert(errorMessage);
    } finally {
      stampingIds[id] = false;
    }
  };

  const viewDetail = (fac: Factura) => {
    selectedFactura = fac;
    showDetailModal = true;
  };

  // Computed Search Filtering
  let filteredFacturas = $derived(
    facturas.filter(f => 
      f.receptor_nombre.toLowerCase().includes(searchQuery.toLowerCase()) ||
      f.receptor_rfc.toLowerCase().includes(searchQuery.toLowerCase()) ||
      f.emisor_nombre.toLowerCase().includes(searchQuery.toLowerCase()) ||
      f.id?.toString() === searchQuery
    )
  );

  const getRegimenDesc = (code: string) => {
    return regimenes.find(r => r.code === code)?.desc || code;
  };

  const getUsoDesc = (code: string) => {
    return usosCfdi.find(u => u.code === code)?.desc || code;
  };
</script>

<div class="container animate-fade-in">
  <header class="page-header">
    <div>
      <h1>Captura de Facturas CFDI 4.0</h1>
      <p class="subtitle">Emisión, cálculo y persistencia de comprobantes fiscales con histórico inalterable</p>
    </div>
    
    {#if !emisorLoading}
      {#if activeEmisor}
        <button class="btn btn-primary" onclick={openCapture}>
          <i class="fa-solid fa-plus"></i> Nueva Factura
        </button>
      {:else}
        <a href="/emisor" class="btn btn-warning">
          <i class="fa-solid fa-triangle-exclamation"></i> Configurar Emisor
        </a>
      {/if}
    {/if}
  </header>

  <!-- Default Emisor display banner -->
  {#if activeEmisor}
    <div class="emisor-badge-banner animate-fade-in">
      <div class="emisor-info">
        <span class="info-label">Facturando como:</span>
        <span class="emisor-name">{activeEmisor.nombre}</span>
        <span class="badge rfc-badge">{activeEmisor.rfc}</span>
        <span class="regimen-txt">{getRegimenDesc(activeEmisor.regimen_fiscal)}</span>
      </div>
    </div>
  {:else if !emisorLoading}
    <div class="alert alert-danger animate-shake">
      <i class="fa-solid fa-circle-exclamation"></i>
      <div>
        <strong>Datos del Emisor requeridos:</strong> No se encuentra configurado ningún Emisor. 
        Vaya a la pestaña <a href="/emisor" style="color: inherit; text-decoration: underline;">Emisor</a> para configurar los datos fiscales de su empresa antes de facturar.
      </div>
    </div>
  {/if}

  <!-- Search and Stats -->
  <div class="search-bar-container">
    <div class="search-input-wrapper">
      <i class="fa-solid fa-magnifying-glass search-icon"></i>
      <input
        type="text"
        placeholder="Buscar por RFC, Cliente o Folio..."
        bind:value={searchQuery}
        class="search-input"
      />
    </div>
    <div class="stats-badge">
      <i class="fa-solid fa-file-invoice"></i> {filteredFacturas.length} Factura{filteredFacturas.length !== 1 ? 's' : ''}
    </div>
  </div>

  <!-- Facturas Table Card -->
  <div class="table-card">
    <div class="table-responsive">
      <table>
        <thead>
          <tr>
            <th>Folio</th>
            <th>Fecha</th>
            <th>Receptor (Cliente)</th>
            <th class="text-right">Subtotal</th>
            <th class="text-right">Impuestos (IVA)</th>
            <th class="text-right">Total</th>
            <th>Estado / UUID</th>
            <th class="actions-column">Acciones</th>
          </tr>
        </thead>
        <tbody>
          {#each filteredFacturas as fac}
            <tr class="table-row">
              <td class="folio-col">
                <strong>FAC-{fac.id?.toString().padStart(4, '0')}</strong>
              </td>
              <td><span class="date-badge"><i class="fa-solid fa-calendar-day"></i> {fac.fecha}</span></td>
              <td>
                <div class="receptor-cell">
                  <span class="receptor-name">{fac.receptor_nombre}</span>
                  <span class="receptor-rfc">{fac.receptor_rfc}</span>
                </div>
              </td>
              <td class="text-right">${fac.subtotal.toFixed(2)}</td>
              <td class="text-right">${fac.impuestos.toFixed(2)}</td>
              <td class="text-right font-bold text-primary">${fac.total.toFixed(2)}</td>
              <td>
                {#if fac.status === 'timbrada'}
                  <div class="status-cell">
                    <span class="badge badge-success"><i class="fa-solid fa-circle-check"></i> Timbrada</span>
                    <span class="uuid-txt" title={fac.uuid}>{fac.uuid?.substring(0, 8)}...</span>
                  </div>
                {:else}
                  <span class="badge badge-secondary"><i class="fa-solid fa-file-pen"></i> Creada</span>
                {/if}
              </td>
              <td class="actions-column">
                <button class="action-btn view" onclick={() => viewDetail(fac)} title="Ver Detalle">
                  <i class="fa-solid fa-eye"></i>
                </button>
                
                {#if fac.status === 'timbrada'}
                  <a href={`http://localhost:8080/factura/${fac.id}/xml`} class="action-btn download" download title="Descargar XML">
                    <i class="fa-solid fa-file-code"></i>
                  </a>
                {:else}
                  {#if stampingIds[fac.id!]}
                    <span class="action-spinner" title="Timbrando..."><i class="fa-solid fa-spinner fa-spin"></i></span>
                  {:else}
                    <button class="action-btn stamp" onclick={() => stampFactura(fac.id!)} title="Timbrar CFDI">
                      <i class="fa-solid fa-signature"></i>
                    </button>
                  {/if}
                {/if}
                
                {#if fac.status !== 'timbrada'}
                  <button class="action-btn delete" onclick={() => deleteFactura(fac.id!)} title="Eliminar">
                    <i class="fa-solid fa-trash"></i>
                  </button>
                {/if}
              </td>
            </tr>
          {:else}
            <tr>
              <td colspan="8" class="empty-state">
                <i class="fa-solid fa-file-excel empty-icon"></i>
                <p>No se encontraron facturas registradas en la base de datos.</p>
              </td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>

<!-- Modal: Capturar CFDI -->
{#if showCaptureModal && activeEmisor}
  <div class="modal-overlay animate-fade-in" onclick={() => showCaptureModal = false} role="presentation">
    <div class="modal-card capture-modal animate-scale-up" onclick={(e) => e.stopPropagation()} role="presentation">
      <header class="modal-header">
        <h2><i class="fa-solid fa-file-signature text-primary"></i> Elaborar Nueva Factura CFDI 4.0</h2>
        <button class="close-btn" onclick={() => showCaptureModal = false}>&times;</button>
      </header>

      <form onsubmit={(e) => { e.preventDefault(); saveFactura(); }}>
        <div class="modal-body overflow-y-auto" style="max-height: 70vh;">
          {#if errorMessage}
            <div class="alert alert-danger animate-shake">
              <i class="fa-solid fa-triangle-exclamation"></i> {errorMessage}
            </div>
          {/if}

          <!-- Block 1: Emisor & Fecha -->
          <div class="form-section">
            <h3 class="section-title"><i class="fa-solid fa-building"></i> 1. Datos del Emisor y Fecha</h3>
            <div class="section-grid">
              <div class="read-only-box">
                <label>Emisor (Tus Datos)</label>
                <div class="box-content">
                  <strong>{activeEmisor.nombre}</strong><br/>
                  <span class="text-xs text-slate-500">RFC: {activeEmisor.rfc} | Régimen: {activeEmisor.regimen_fiscal}</span>
                </div>
              </div>
              <div class="form-group">
                <label for="fecha">Fecha del Día</label>
                <input
                  id="fecha"
                  type="date"
                  bind:value={invoiceDate}
                  required
                  class="form-control"
                />
                {#if dateWarning}
                  <span class="warning-text"><i class="fa-solid fa-triangle-exclamation"></i> {dateWarning}</span>
                {/if}
              </div>
            </div>
          </div>

          <!-- Block 2: Receptor -->
          <div class="form-section">
            <h3 class="section-title"><i class="fa-solid fa-user-tie"></i> 2. Seleccionar Receptor (Cliente)</h3>
            <div class="form-group">
              <label for="receptor">Cliente (Receptor)</label>
              <select id="receptor" bind:value={selectedReceptorId} class="form-control" required>
                <option value="">-- Seleccione un Receptor del catálogo --</option>
                {#each receptores as r}
                  <option value={r.id?.toString()}>{r.nombre} ({r.rfc})</option>
                {/each}
              </select>
            </div>
            
            {#if selectedReceptor}
              <div class="details-card-flat animate-fade-in">
                <div class="details-grid">
                  <div>
                    <span class="details-label">RFC</span>
                    <span class="details-val font-mono">{selectedReceptor.rfc}</span>
                  </div>
                  <div>
                    <span class="details-label">Régimen Fiscal</span>
                    <span class="details-val">{getRegimenDesc(selectedReceptor.regimen_fiscal_receptor)}</span>
                  </div>
                  <div>
                    <span class="details-label">Domicilio Fiscal (C.P.)</span>
                    <span class="details-val">{selectedReceptor.domicilio_fiscal_receptor}</span>
                  </div>
                  <div>
                    <span class="details-label">Uso CFDI</span>
                    <span class="details-val">{getUsoDesc(selectedReceptor.uso_cfdi)}</span>
                  </div>
                </div>
              </div>
            {/if}
          </div>

          <!-- Block 3: Conceptos (Conceptos Builder) -->
          <div class="form-section">
            <h3 class="section-title"><i class="fa-solid fa-list-check"></i> 3. Conceptos de la Factura</h3>
            
            <!-- Add concept sub-form -->
            <div class="concept-input-form">
              <h4>Agregar Concepto de Catálogo</h4>
              <div class="concept-input-grid">
                <div class="form-group col-span-3">
                  <label for="predef_concept">Concepto Catálogo</label>
                  <select id="predef_concept" bind:value={selectedPredefinedId} class="form-control">
                    <option value="">-- Buscar / Seleccionar concepto --</option>
                    {#each predefinedConceptos as c}
                      <option value={c.id?.toString()}>{c.descripcion} (${c.valor_unitario.toFixed(2)})</option>
                    {/each}
                  </select>
                </div>

                <div class="form-group">
                  <label for="line_cant">Cantidad</label>
                  <input
                    id="line_cant"
                    type="number"
                    min="0.0001"
                    step="any"
                    bind:value={lineCantidad}
                    class="form-control"
                  />
                </div>

                <div class="form-group col-span-2">
                  <label for="line_desc">Descripción en Factura</label>
                  <input
                    id="line_desc"
                    type="text"
                    bind:value={lineDescripcion}
                    class="form-control"
                    placeholder="Descripción personalizada..."
                  />
                </div>

                <div class="form-group">
                  <label for="line_val">Precio Unitario ($)</label>
                  <input
                    id="line_val"
                    type="number"
                    min="0"
                    step="0.0001"
                    bind:value={lineValorUnitario}
                    class="form-control"
                  />
                </div>

                <div class="form-group">
                  <label for="line_iva">IVA</label>
                  <select id="line_iva" bind:value={lineIvaPorcentaje} class="form-control">
                    <option value={0}>0%</option>
                    <option value={8}>8%</option>
                    <option value={16}>16%</option>
                  </select>
                </div>

                <div class="form-group">
                  <label for="line_clave">Clave SAT</label>
                  <input
                    id="line_clave"
                    type="text"
                    maxlength="8"
                    bind:value={lineClaveProd}
                    class="form-control"
                    placeholder="Ej: 15101514"
                  />
                </div>

                <div class="form-group">
                  <label style="visibility: hidden;">&nbsp;</label>
                  <button type="button" class="btn btn-secondary-outline btn-full-height" onclick={addConceptLine}>
                    <i class="fa-solid fa-plus"></i> Agregar
                  </button>
                </div>
              </div>

              <!-- Line preview numbers -->
              {#if lineDescripcion}
                <div class="line-preview-banner">
                  <span>Subtotal línea: <strong>${computedLineSubtotal.toFixed(2)}</strong></span>
                  <span>IVA ({lineIvaPorcentaje}%): <strong>${computedLineImpuesto.toFixed(2)}</strong></span>
                  <span>Total línea: <strong>${computedLineTotal.toFixed(2)}</strong></span>
                </div>
              {/if}
            </div>

            <!-- Concepts Added Table -->
            <div class="added-concepts-table-container">
              <h5>Conceptos agregados a la factura:</h5>
              <table class="concepts-table">
                <thead>
                  <tr>
                    <th>SAT</th>
                    <th>Descripción</th>
                    <th class="text-center">Cant.</th>
                    <th class="text-right">P. Unitario</th>
                    <th class="text-center">IVA</th>
                    <th class="text-right">Subtotal</th>
                    <th class="text-right">Total</th>
                    <th style="width: 50px;"></th>
                  </tr>
                </thead>
                <tbody>
                  {#each currentConceptos as concept, index}
                    <tr>
                      <td class="font-mono text-xs">{concept.clave_prod_serv}</td>
                      <td>{concept.descripcion}</td>
                      <td class="text-center">{concept.cantidad}</td>
                      <td class="text-right">${concept.valor_unitario.toFixed(2)}</td>
                      <td class="text-center"><span class="iva-badge">{concept.iva_porcentaje}%</span></td>
                      <td class="text-right">${concept.subtotal.toFixed(2)}</td>
                      <td class="text-right font-bold">${concept.total.toFixed(2)}</td>
                      <td>
                        <button type="button" class="icon-only-btn text-danger" onclick={() => removeConceptLine(index)}>
                          <i class="fa-solid fa-circle-minus"></i>
                        </button>
                      </td>
                    </tr>
                  {:else}
                    <tr>
                      <td colspan="8" class="empty-concepts">
                        No se han agregado conceptos. Utilice el formulario superior para añadir elementos.
                      </td>
                    </tr>
                  {/each}
                </tbody>
              </table>
            </div>
          </div>

          <!-- Invoice Totals Section -->
          <div class="totals-calculation-box">
            <div class="total-row">
              <span class="label">Subtotal:</span>
              <span class="value">${calculatedSubtotal.toFixed(2)}</span>
            </div>
            <div class="total-row">
              <span class="label">Impuestos Trasladados (IVA):</span>
              <span class="value">${calculatedImpuestos.toFixed(2)}</span>
            </div>
            <div class="total-row main-total">
              <span class="label">TOTAL FACTURA:</span>
              <span class="value">${calculatedTotal.toFixed(2)} MXN</span>
            </div>
          </div>
        </div>

        <footer class="modal-footer">
          <button type="button" class="btn btn-secondary" onclick={() => showCaptureModal = false}>Cancelar</button>
          <button type="submit" class="btn btn-primary" disabled={currentConceptos.length === 0 || !selectedReceptor}>
            <i class="fa-solid fa-floppy-disk"></i> Guardar y Emitir Factura
          </button>
        </footer>
      </form>
    </div>
  </div>
{/if}

<!-- Modal: Ver Detalle Factura -->
{#if showDetailModal && selectedFactura}
  <div class="modal-overlay animate-fade-in" onclick={() => showDetailModal = false} role="presentation">
    <div class="modal-card detail-modal animate-scale-up" onclick={(e) => e.stopPropagation()} role="presentation">
      <header class="modal-header">
        <h2>CFDI Detalle - Folio FAC-{selectedFactura.id?.toString().padStart(4, '0')}</h2>
        <button class="close-btn" onclick={() => showDetailModal = false}>&times;</button>
      </header>

      <div class="modal-body overflow-y-auto" style="max-height: 70vh;">
        <!-- Emisor and Receptor blocks side-by-side -->
        <div class="detail-fiscal-grid">
          <div class="detail-party-card">
            <h4><i class="fa-solid fa-building text-primary"></i> Datos del Emisor</h4>
            <div class="party-info">
              <span class="party-label">Nombre / Razón Social</span>
              <span class="party-val highlight">{selectedFactura.emisor_nombre}</span>
              
              <span class="party-label">RFC</span>
              <span class="party-val font-mono">{selectedFactura.emisor_rfc}</span>
              
              <span class="party-label">Régimen Fiscal</span>
              <span class="party-val">{getRegimenDesc(selectedFactura.emisor_regimen_fiscal)}</span>
            </div>
          </div>

          <div class="detail-party-card">
            <h4><i class="fa-solid fa-user-tie text-primary"></i> Datos del Receptor</h4>
            <div class="party-info">
              <span class="party-label">Nombre / Razón Social</span>
              <span class="party-val highlight">{selectedFactura.receptor_nombre}</span>
              
              <span class="party-label">RFC</span>
              <span class="party-val font-mono">{selectedFactura.receptor_rfc}</span>

              <span class="party-label">Domicilio Fiscal (C.P.)</span>
              <span class="party-val">{selectedFactura.receptor_domicilio_fiscal}</span>
              
              <span class="party-label">Régimen Fiscal</span>
              <span class="party-val">{getRegimenDesc(selectedFactura.receptor_regimen_fiscal)}</span>

              <span class="party-label">Uso de CFDI</span>
              <span class="party-val">{getUsoDesc(selectedFactura.receptor_uso_cfdi)}</span>
            </div>
          </div>
        </div>

        <!-- Meta invoice info -->
        <div class="meta-detail-row">
          <div>
            <span class="party-label">Fecha de Emisión</span>
            <span class="party-val font-bold"><i class="fa-solid fa-calendar"></i> {selectedFactura.fecha}</span>
          </div>
          <div>
            <span class="party-label">Tipo de Comprobante</span>
            <span class="party-val"><span class="badge type-badge">I - Ingreso</span></span>
          </div>
          <div>
            <span class="party-label">Versión CFDI</span>
            <span class="party-val"><span class="badge version-badge">SAT CFDI 4.0</span></span>
          </div>
        </div>

        <!-- Concept list table -->
        <div class="concept-detail-table-wrapper">
          <h4>Conceptos Declarados</h4>
          <table class="concepts-table">
            <thead>
              <tr>
                <th>Clave SAT</th>
                <th>Clave Unidad</th>
                <th>Descripción del Concepto</th>
                <th class="text-center">Cant.</th>
                <th class="text-right">Valor Unitario</th>
                <th class="text-center">IVA</th>
                <th class="text-right">Subtotal</th>
                <th class="text-right">Total</th>
              </tr>
            </thead>
            <tbody>
              {#if Array.isArray(selectedFactura.conceptos)}
                {#each selectedFactura.conceptos as item}
                  <tr>
                    <td class="font-mono text-xs">{item.clave_prod_serv}</td>
                    <td>{item.clave_unidad}</td>
                    <td>{item.descripcion}</td>
                    <td class="text-center">{item.cantidad}</td>
                    <td class="text-right">${item.valor_unitario.toFixed(4)}</td>
                    <td class="text-center"><span class="iva-badge">{item.iva_porcentaje}%</span></td>
                    <td class="text-right">${item.subtotal.toFixed(2)}</td>
                    <td class="text-right font-bold">${item.total.toFixed(2)}</td>
                  </tr>
                {/each}
              {/if}
            </tbody>
          </table>
        </div>

        <!-- Summary Totals box -->
        <div class="totals-calculation-box">
          <div class="total-row">
            <span class="label">Subtotal:</span>
            <span class="value">${selectedFactura.subtotal.toFixed(2)}</span>
          </div>
          <div class="total-row">
            <span class="label">Impuestos Trasladados (IVA):</span>
            <span class="value">${selectedFactura.impuestos.toFixed(2)}</span>
          </div>
          <div class="total-row main-total">
            <span class="label">TOTAL GENERAL:</span>
            <span class="value">${selectedFactura.total.toFixed(2)} MXN</span>
          </div>
        </div>
      </div>

      <footer class="modal-footer">
        <button type="button" class="btn btn-secondary" onclick={() => showDetailModal = false}>Cerrar</button>
      </footer>
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
    margin-bottom: 24px;
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

  /* Emisor Banner Badge */
  .emisor-badge-banner {
    background: #f1f5f9;
    border-radius: 10px;
    padding: 12px 20px;
    border: 1px solid #e2e8f0;
    margin-bottom: 24px;
  }

  .emisor-info {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
    font-size: 0.9rem;
    color: #475569;
  }

  .info-label {
    font-weight: 600;
    color: #64748b;
  }

  .emisor-name {
    font-weight: 800;
    color: #0f172a;
  }

  .regimen-txt {
    color: #64748b;
    font-size: 0.85rem;
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

  /* Table styling */
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

  .text-right {
    text-align: right;
  }

  .text-center {
    text-align: center;
  }

  .font-bold {
    font-weight: 700;
  }

  .table-row {
    transition: background-color 0.2s;
  }

  .table-row:hover {
    background-color: #f8fafc;
  }

  .folio-col {
    color: #334155;
  }

  .date-badge {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 4px 8px;
    background: #f1f5f9;
    border-radius: 6px;
    font-size: 0.85rem;
    color: #475569;
    font-weight: 600;
  }

  .receptor-cell {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .receptor-name {
    font-weight: 600;
    color: #0f172a;
  }

  .receptor-rfc {
    font-family: monospace;
    font-size: 0.8rem;
    color: #64748b;
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
  }

  .text-primary {
    color: #4facfe;
  }

  .empty-state {
    text-align: center;
    padding: 64px !important;
    color: #a0aec0;
  }

  .empty-icon {
    font-size: 2.5rem;
    margin-bottom: 12px;
    color: #cbd5e0;
  }

  /* Actions column */
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

  .action-btn.view {
    color: #4facfe;
  }

  .action-btn.view:hover {
    background: rgba(79, 172, 254, 0.1);
  }

  .action-btn.delete {
    color: #e53e3e;
  }

  .action-btn.delete:hover {
    background: rgba(229, 62, 62, 0.1);
  }

  /* Capture modal customizations */
  .capture-modal {
    max-width: 950px;
  }

  .detail-modal {
    max-width: 850px;
  }

  /* Form layouts */
  .form-section {
    border-bottom: 1px solid #edf2f7;
    padding-bottom: 24px;
    margin-bottom: 24px;
  }

  .form-section:last-of-type {
    border-bottom: none;
    padding-bottom: 0;
  }

  .section-title {
    font-size: 1.1rem;
    font-weight: 700;
    color: #1e293b;
    margin: 0 0 16px 0;
    display: flex;
    align-items: center;
    gap: 8px;
    border-left: 3px solid #4facfe;
    padding-left: 8px;
  }

  .section-grid {
    display: grid;
    grid-template-columns: 1.5fr 1fr;
    gap: 20px;
  }

  .read-only-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 12px;
  }

  .read-only-box label {
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    color: #94a3b8;
    margin-bottom: 6px;
    display: block;
  }

  .box-content {
    color: #1e293b;
    font-size: 0.95rem;
  }

  .warning-text {
    color: #ea580c;
    font-size: 0.8rem;
    margin-top: 4px;
    display: block;
    font-weight: 500;
  }

  /* Receptor selected info flat card */
  .details-card-flat {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 14px;
    margin-top: 12px;
  }

  .details-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 12px;
  }

  @media (max-width: 600px) {
    .details-grid {
      grid-template-columns: 1fr 1fr;
    }
  }

  .details-label {
    display: block;
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    color: #94a3b8;
    margin-bottom: 2px;
  }

  .details-val {
    font-size: 0.85rem;
    color: #334155;
    font-weight: 600;
  }

  /* Concepts capture block */
  .concept-input-form {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 16px;
    margin-bottom: 20px;
  }

  .concept-input-form h4 {
    margin: 0 0 14px 0;
    font-size: 0.95rem;
    font-weight: 700;
    color: #334155;
  }

  .concept-input-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 14px;
  }

  .col-span-2 {
    grid-column: span 2;
  }

  .col-span-3 {
    grid-column: span 3;
  }

  @media (max-width: 768px) {
    .concept-input-grid {
      grid-template-columns: 1fr 1fr;
    }
    .col-span-3 {
      grid-column: span 2;
    }
  }

  .btn-full-height {
    height: 40px;
    width: 100%;
    justify-content: center;
  }


  .line-preview-banner {
    background: #e0f2fe;
    color: #0369a1;
    padding: 8px 14px;
    border-radius: 6px;
    margin-top: 14px;
    font-size: 0.85rem;
    display: flex;
    gap: 20px;
  }

  /* Added concepts table */
  .added-concepts-table-container {
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    overflow: hidden;
  }

  .added-concepts-table-container h5 {
    margin: 12px 16px;
    font-size: 0.9rem;
    font-weight: 700;
    color: #475569;
  }

  .concepts-table {
    width: 100%;
    border-collapse: collapse;
  }

  .concepts-table th {
    padding: 10px 16px;
    background: #f1f5f9;
    color: #475569;
    font-size: 0.8rem;
    border-bottom: 1px solid #cbd5e1;
  }

  .concepts-table td {
    padding: 10px 16px;
    font-size: 0.9rem;
    border-bottom: 1px solid #e2e8f0;
  }

  .empty-concepts {
    text-align: center;
    padding: 24px !important;
    color: #94a3b8;
    font-style: italic;
  }

  .iva-badge {
    background: #e2e8f0;
    color: #334155;
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 0.75rem;
    font-weight: 600;
  }

  .icon-only-btn {
    background: none;
    border: none;
    cursor: pointer;
    font-size: 1.2rem;
    padding: 4px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: transform 0.2s;
  }

  .icon-only-btn:hover {
    transform: scale(1.15);
  }

  .text-danger {
    color: #ef4444;
  }

  /* Totals Calculation Box */
  .totals-calculation-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    color: #1e293b;
    border-radius: 12px;
    padding: 20px 24px;
    margin-top: 24px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    max-width: 400px;
    margin-left: auto;
    box-shadow: 0 2px 8px rgba(0,0,0,0.04);
  }

  .total-row {
    display: flex;
    justify-content: space-between;
    font-size: 0.95rem;
    border-bottom: 1px solid #e2e8f0;
    padding-bottom: 8px;
  }

  .total-row .label {
    color: #64748b;
  }

  .total-row .value {
    font-weight: 600;
    color: #0f172a;
  }

  .total-row.main-total {
    border-bottom: none;
    padding-bottom: 0;
    font-size: 1.3rem;
    font-weight: 800;
  }

  .total-row.main-total .label {
    color: #0284c7;
  }

  .total-row.main-total .value {
    color: #0284c7;
  }

  /* Verification Detail Modal styles */
  .detail-fiscal-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
    margin-bottom: 20px;
  }

  @media (max-width: 600px) {
    .detail-fiscal-grid {
      grid-template-columns: 1fr;
    }
  }

  .detail-party-card {
    border: 1px solid #edf2f7;
    background: #f8fafc;
    border-radius: 10px;
    padding: 16px;
  }

  .detail-party-card h4 {
    margin: 0 0 14px 0;
    font-size: 1rem;
    font-weight: 700;
    color: #1a202c;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .party-info {
    display: grid;
    grid-template-columns: 1fr;
    gap: 6px;
  }

  .party-label {
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    color: #94a3b8;
    margin-top: 6px;
  }

  .party-label:first-of-type {
    margin-top: 0;
  }

  .party-val {
    font-size: 0.9rem;
    font-weight: 600;
    color: #334155;
  }

  .party-val.highlight {
    font-size: 1.05rem;
    font-weight: 800;
    color: #0f172a;
  }

  .meta-detail-row {
    background: #f1f5f9;
    border-radius: 8px;
    padding: 12px 20px;
    display: flex;
    justify-content: space-between;
    margin-bottom: 20px;
    gap: 12px;
  }

  .meta-detail-row div {
    display: flex;
    flex-direction: column;
  }

  .type-badge {
    background: #dcfce7;
    color: #15803d;
  }

  .version-badge {
    background: #e0f2fe;
    color: #0369a1;
  }

  .concept-detail-table-wrapper {
    margin-top: 24px;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    overflow: hidden;
  }

  .concept-detail-table-wrapper h4 {
    margin: 12px 16px;
    font-size: 0.95rem;
    font-weight: 700;
    color: #334155;
  }

  /* Utility classes */
  .btn-warning {
    background: #f59e0b;
    color: #fff;
    box-shadow: 0 4px 10px rgba(245, 158, 11, 0.3);
  }

  .btn-warning:hover {
    background: #d97706;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    gap: 6px;
    margin-bottom: 12px;
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

  .btn-secondary-outline {
    background: transparent;
    border: 1px solid #cbd5e1;
    color: #475569;
  }

  .btn-secondary-outline:hover {
    background: #f8fafc;
    border-color: #94a3b8;
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

  /* Modals generic */
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

  .overflow-y-auto {
    overflow-y: auto;
  }

  .modal-footer {
    padding: 16px 24px;
    border-top: 1px solid #edf2f7;
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    background: #f8fafc;
  }

  .badge-success {
    background-color: rgba(34, 197, 94, 0.1);
    color: #22c55e;
    border: 1px solid rgba(34, 197, 94, 0.2);
  }
  
  .badge-secondary {
    background-color: rgba(100, 116, 139, 0.1);
    color: #64748b;
    border: 1px solid rgba(100, 116, 139, 0.2);
  }
  
  .status-cell {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 4px;
  }
  
  .uuid-txt {
    font-size: 0.75rem;
    font-family: monospace;
    color: #64748b;
  }
  
  .action-btn.stamp {
    color: #3b82f6;
    background-color: rgba(59, 130, 246, 0.1);
    border: 1px solid rgba(59, 130, 246, 0.2);
  }
  
  .action-btn.stamp:hover {
    background-color: #3b82f6;
    color: #ffffff;
  }
  
  .action-btn.download {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    text-decoration: none;
    color: #10b981;
    background-color: rgba(16, 185, 129, 0.1);
    border: 1px solid rgba(16, 185, 129, 0.2);
    width: 2.25rem;
    height: 2.25rem;
    border-radius: 0.375rem;
    cursor: pointer;
    transition: all 0.2s ease;
  }
  
  .action-btn.download:hover {
    background-color: #10b981;
    color: #ffffff;
  }
  
  .action-spinner {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    color: #3b82f6;
    width: 2.25rem;
    height: 2.25rem;
  }

  /* Animations */
  .animate-fade-in {
    animation: fadeIn 0.25s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  .animate-scale-up {
    animation: scaleUp 0.3s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
  }

  .animate-shake {
    animation: shake 0.35s ease-in-out;
  }

  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  @keyframes scaleUp {
    from { transform: scale(0.95); opacity: 0; }
    to { transform: scale(1); opacity: 1; }
  }

  @keyframes shake {
    0%, 100% { transform: translateX(0); }
    25% { transform: translateX(-4px); }
    75% { transform: translateX(4px); }
  }
</style>
