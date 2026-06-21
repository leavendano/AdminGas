<script lang="ts">
  import { onMount } from "svelte";

  interface Emisor {
    id?: number;
    rfc: string;
    nombre: string;
    regimen_fiscal: string;
    domicilio_fiscal?: string;
    certificado?: string;
    llave?: string;
    certificado_nombre?: string;
    llave_nombre?: string;
    llave_password?: string;
    certificado_valido_desde?: string;
    certificado_valido_hasta?: string;
    certificado_numero?: string;
  }

  let emisor = $state<Emisor | null>(null);
  let isEditing = $state(false);

  // Form states
  let rfc = $state("");
  let nombre = $state("");
  let regimenFiscal = $state("601");
  let domicilioFiscal = $state("");
  let errorMessage = $state("");
  let successMessage = $state("");

  // CSD state
  let cerFile = $state<File | null>(null);
  let keyFile = $state<File | null>(null);
  let passwordCSD = $state("");
  let showPassword = $state(false);
  let isUploadingCSD = $state(false);
  let csdErrorMessage = $state("");
  let csdSuccessMessage = $state("");
  let showCsdForm = $state(false);
  let isDraggingCer = $state(false);
  let isDraggingKey = $state(false);

  // SAT Regimenes for Emitters
  const regimenes = [
    { code: "601", desc: "601 - General de Ley Personas Morales" },
    { code: "603", desc: "603 - Personas Morales con Fines no Lucrativos" },
    {
      code: "605",
      desc: "605 - Sueldos y Salarios e Ingresos Asimilados a Salarios",
    },
    { code: "606", desc: "606 - Arrendamiento" },
    {
      code: "612",
      desc: "612 - Personas Físicas con Actividades Empresariales y Profesionales",
    },
    { code: "621", desc: "621 - Incorporación Fiscal" },
    { code: "626", desc: "626 - Régimen Simplificado de Confianza (RESICO)" },
  ];

  const loadEmisor = async () => {
    try {
      const response = await fetch("http://localhost:8080/emisor");
      if (response.ok) {
        const list: Emisor[] = await response.json();
        if (list.length > 0) {
          emisor = list[0]; // Take the first record as the active company configuration
          rfc = emisor.rfc;
          nombre = emisor.nombre;
          regimenFiscal = emisor.regimen_fiscal;
          domicilioFiscal = emisor.domicilio_fiscal || "";
          passwordCSD = emisor.llave_password || "";
        } else {
          emisor = null;
          isEditing = true; // No record yet, show form directly
        }
      }
    } catch (error) {
      console.error("Error cargando emisor:", error);
    }
  };

  onMount(() => {
    loadEmisor();

    // Prevent default drag/drop behavior globally to stop browser downloads
    const preventGlobalDefault = (e: DragEvent) => {
      e.preventDefault();
    };
    window.addEventListener("dragover", preventGlobalDefault);
    window.addEventListener("drop", preventGlobalDefault);

    return () => {
      window.removeEventListener("dragover", preventGlobalDefault);
      window.removeEventListener("drop", preventGlobalDefault);
    };
  });

  const validateForm = () => {
    const cleanRfc = rfc.trim().toUpperCase();
    if (!cleanRfc) return "El RFC es requerido";
    if (cleanRfc.length < 12 || cleanRfc.length > 13)
      return "El RFC debe tener 12 o 13 caracteres";

    const rfcRegex = /^[A-Z&Ñ]{3,4}[0-9]{6}[A-Z0-9]{3}$/;
    if (!rfcRegex.test(cleanRfc)) return "Formato de RFC inválido";

    if (!nombre.trim()) return "El Nombre o Razón Social es requerido";

    const cleanCp = domicilioFiscal.trim();
    if (!cleanCp) return "El Domicilio Fiscal (Código Postal) es requerido";
    if (cleanCp.length !== 5 || !/^\d{5}$/.test(cleanCp))
      return "El Domicilio Fiscal (Código Postal) debe tener exactamente 5 dígitos numéricos";

    return null;
  };

  const saveEmisor = async () => {
    errorMessage = "";
    successMessage = "";

    const error = validateForm();
    if (error) {
      errorMessage = error;
      return;
    }

    const payload: Emisor = {
      rfc: rfc.trim().toUpperCase(),
      nombre: nombre.trim().toUpperCase(),
      regimen_fiscal: regimenFiscal,
      domicilio_fiscal: domicilioFiscal.trim(),
    };

    try {
      let response;
      if (emisor && emisor.id) {
        response = await fetch(`http://localhost:8080/emisor/${emisor.id}`, {
          method: "PUT",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
      } else {
        response = await fetch("http://localhost:8080/emisor", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
      }

      if (response.ok) {
        successMessage = "Datos del emisor actualizados correctamente";
        isEditing = false;
        await loadEmisor();
      } else {
        const data = await response.json();
        errorMessage = data.error || "Error al guardar los datos";
      }
    } catch (err) {
      console.error(err);
      errorMessage = "Error de conexión con el servidor";
    }
  };

  const getRegimenDesc = (code: string) => {
    return regimenes.find((r) => r.code === code)?.desc || code;
  };

  const handleCerChange = (e: Event) => {
    const input = e.target as HTMLInputElement;
    if (input.files && input.files[0]) {
      const file = input.files[0];
      if (!file.name.endsWith(".cer")) {
        csdErrorMessage = "El archivo del certificado debe ser de tipo .cer";
        cerFile = null;
        input.value = "";
      } else {
        csdErrorMessage = "";
        cerFile = file;
      }
    }
  };

  const handleKeyChange = (e: Event) => {
    const input = e.target as HTMLInputElement;
    if (input.files && input.files[0]) {
      const file = input.files[0];
      if (!file.name.endsWith(".key")) {
        csdErrorMessage = "El archivo de la llave privada debe ser de tipo .key";
        keyFile = null;
        input.value = "";
      } else {
        csdErrorMessage = "";
        keyFile = file;
      }
    }
  };

  const handleCerDrop = (e: DragEvent) => {
    e.preventDefault();
    isDraggingCer = false;
    if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0]) {
      const file = e.dataTransfer.files[0];
      if (!file.name.endsWith(".cer")) {
        csdErrorMessage = "El archivo del certificado debe ser de tipo .cer";
        cerFile = null;
      } else {
        csdErrorMessage = "";
        cerFile = file;
      }
    }
  };

  const handleKeyDrop = (e: DragEvent) => {
    e.preventDefault();
    isDraggingKey = false;
    if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0]) {
      const file = e.dataTransfer.files[0];
      if (!file.name.endsWith(".key")) {
        csdErrorMessage = "El archivo de la llave privada debe ser de tipo .key";
        keyFile = null;
      } else {
        csdErrorMessage = "";
        keyFile = file;
      }
    }
  };

  const fileToBase64 = (file: File): Promise<string> => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.readAsDataURL(file);
      reader.onload = () => {
        const base64 = (reader.result as string).split(",")[1];
        resolve(base64);
      };
      reader.onerror = (error) => reject(error);
    });
  };

  const saveCertificados = async () => {
    csdErrorMessage = "";
    csdSuccessMessage = "";

    if (!emisor || !emisor.id) {
      csdErrorMessage = "Primero debes configurar y guardar los datos fiscales del emisor.";
      return;
    }

    if (!cerFile && !emisor.certificado_nombre) {
      csdErrorMessage = "El archivo .cer es requerido.";
      return;
    }
    if (!keyFile && !emisor.llave_nombre) {
      csdErrorMessage = "El archivo .key es requerido.";
      return;
    }
    if (!passwordCSD.trim()) {
      csdErrorMessage = "La contraseña del certificado CSD es requerida.";
      return;
    }

    isUploadingCSD = true;
    try {
      const payload: Partial<Emisor> = {
        rfc: emisor.rfc,
        nombre: emisor.nombre,
        regimen_fiscal: emisor.regimen_fiscal,
        domicilio_fiscal: emisor.domicilio_fiscal,
        llave_password: passwordCSD.trim(),
      };

      if (cerFile) {
        payload.certificado = await fileToBase64(cerFile);
        payload.certificado_nombre = cerFile.name;
      }
      if (keyFile) {
        payload.llave = await fileToBase64(keyFile);
        payload.llave_nombre = keyFile.name;
      }

      const response = await fetch(`http://localhost:8080/emisor/${emisor.id}`, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });

      if (response.ok) {
        csdSuccessMessage = "Certificados CSD actualizados correctamente";
        cerFile = null;
        keyFile = null;
        showCsdForm = false;
        await loadEmisor();
      } else {
        const data = await response.json();
        csdErrorMessage = data.error || "Error al subir los certificados";
      }
    } catch (err) {
      console.error(err);
      csdErrorMessage = "Error de conexión al subir certificados";
    } finally {
      isUploadingCSD = false;
    }
  };
</script>

<div class="container animate-fade-in">
  <header class="page-header">
    <div>
      <h1>Datos del Emisor (Tu Empresa)</h1>
      <p class="subtitle">
        Configura los datos fiscales de tu empresa necesarios para emitir
        comprobantes CFDI 4.0
      </p>
    </div>
  </header>

  <div class="emisor-layout">
    <div class="main-column">
      <!-- Main Configuration Form or Display -->
      <div class="main-card">
        <div class="card-header">
          <h2><i class="fa-solid fa-building"></i> Ficha de Datos Fiscales</h2>
          {#if !isEditing && emisor}
            <button
              class="btn btn-secondary-outline"
              onclick={() => (isEditing = true)}
            >
              <i class="fa-solid fa-pen"></i> Editar Datos
            </button>
          {/if}
        </div>

        <div class="card-body">
          {#if successMessage}
            <div class="alert alert-success animate-fade-in">
              <i class="fa-solid fa-circle-check"></i>
              {successMessage}
            </div>
          {/if}

          {#if errorMessage}
            <div class="alert alert-danger animate-shake">
              <i class="fa-solid fa-triangle-exclamation"></i>
              {errorMessage}
            </div>
          {/if}

          {#if !isEditing && emisor}
            <!-- Display mode -->
            <div class="data-display animate-fade-in">
              <div class="display-group">
                <span class="label">Razón Social</span>
                <span class="value company-name">{emisor.nombre}</span>
              </div>

              <div class="display-grid">
                <div class="display-group">
                  <span class="label">RFC del Emisor</span>
                  <span class="value code-font">{emisor.rfc}</span>
                </div>

                <div class="display-group">
                  <span class="label">Régimen Fiscal</span>
                  <span class="value"
                    >{getRegimenDesc(emisor.regimen_fiscal)}</span
                  >
                </div>

                <div class="display-group">
                  <span class="label">Domicilio Fiscal (C.P.)</span>
                  <span class="value code-font">{emisor.domicilio_fiscal || 'No configurado'}</span>
                </div>
              </div>

              <div class="status-indicator configured">
                <i class="fa-solid fa-circle-check"></i> Emisor activo y listo para
                facturar
              </div>
            </div>
          {:else}
            <!-- Form/Edit mode -->
            <form
              onsubmit={(e) => {
                e.preventDefault();
                saveEmisor();
              }}
              class="animate-fade-in"
            >
              <div class="form-grid">
                <div class="form-group col-span-2">
                  <label for="nombre"
                    >Nombre o Razón Social (Mayúsculas, tal cual aparece en la
                    constancia del SAT)</label
                  >
                  <input
                    id="nombre"
                    type="text"
                    bind:value={nombre}
                    placeholder="EJ: SERVICIOS GASOLINEROS DE MEXICO"
                    required
                    class="form-control uppercase"
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
                  <label for="regimen">Régimen Fiscal</label>
                  <select
                    id="regimen"
                    bind:value={regimenFiscal}
                    class="form-control"
                  >
                    {#each regimenes as r}
                      <option value={r.code}>{r.desc}</option>
                    {/each}
                  </select>
                </div>

                <div class="form-group">
                  <label for="domicilio-fiscal">Domicilio Fiscal (Código Postal)</label>
                  <input
                    id="domicilio-fiscal"
                    type="text"
                    bind:value={domicilioFiscal}
                    placeholder="Código Postal de 5 dígitos"
                    maxlength="5"
                    required
                    class="form-control"
                  />
                </div>
              </div>

              <div class="form-actions">
                {#if emisor}
                  <button
                    type="button"
                    class="btn btn-secondary"
                    onclick={() => {
                      isEditing = false;
                      errorMessage = "";
                    }}
                  >
                    Cancelar
                  </button>
                {/if}
                <button type="submit" class="btn btn-primary">
                  <i class="fa-solid fa-floppy-disk"></i> Guardar Cambios
                </button>
              </div>
            </form>
          {/if}
        </div>
      </div>

      <!-- CSD Configuration Card -->
      <div class="csd-card">
        <div class="card-header">
          <h2><i class="fa-solid fa-file-shield"></i> Certificados de Sello Digital (CSD)</h2>
          {#if emisor && (emisor.certificado_nombre || emisor.llave_nombre) && !showCsdForm}
            <button
              class="btn btn-secondary-outline"
              onclick={() => {
                showCsdForm = true;
                csdErrorMessage = "";
                csdSuccessMessage = "";
              }}
            >
              <i class="fa-solid fa-arrows-rotate"></i> Reemplazar Certificados
            </button>
          {/if}
        </div>

        <div class="card-body">
          {#if csdSuccessMessage}
            <div class="alert alert-success animate-fade-in">
              <i class="fa-solid fa-circle-check"></i>
              {csdSuccessMessage}
            </div>
          {/if}

          {#if csdErrorMessage}
            <div class="alert alert-danger animate-shake">
              <i class="fa-solid fa-triangle-exclamation"></i>
              {csdErrorMessage}
            </div>
          {/if}

          {#if !emisor}
            <div class="status-indicator-warning">
              <i class="fa-solid fa-triangle-exclamation"></i> Debes registrar los datos fiscales antes de poder agregar certificados CSD.
            </div>
          {:else if !showCsdForm && emisor.certificado_nombre && emisor.llave_nombre}
            <!-- Status mode -->
            <div class="data-display animate-fade-in">
              <div class="csd-status-badges">
                <div class="badge-item">
                  <div class="badge-label">
                    <i class="fa-solid fa-certificate file-icon active"></i>
                    <span>Certificado (.cer)</span>
                  </div>
                  <span class="badge-value loaded">Cargado</span>
                </div>
                <div class="display-group" style="padding-left: 36px; margin-top: -8px; margin-bottom: 8px;">
                  <span class="label">Archivo</span>
                  <span class="value code-font" style="font-size: 0.9rem;">{emisor.certificado_nombre}</span>
                </div>

                <div class="badge-item">
                  <div class="badge-label">
                    <i class="fa-solid fa-key file-icon active"></i>
                    <span>Llave Privada (.key)</span>
                  </div>
                  <span class="badge-value loaded">Cargada</span>
                </div>
                <div class="display-group" style="padding-left: 36px; margin-top: -8px; margin-bottom: 8px;">
                  <span class="label">Archivo</span>
                  <span class="value code-font" style="font-size: 0.9rem;">{emisor.llave_nombre}</span>
                </div>

                {#if emisor.certificado_numero}
                  <div class="badge-item">
                    <div class="badge-label">
                      <i class="fa-solid fa-hashtag file-icon active"></i>
                      <span>No. de Certificado</span>
                    </div>
                    <span class="value code-font" style="font-weight: 700; color: #1e293b; font-size: 0.95rem;">{emisor.certificado_numero}</span>
                  </div>
                {/if}

                {#if emisor.certificado_valido_desde && emisor.certificado_valido_hasta}
                  <div class="badge-item" style="flex-direction: column; align-items: flex-start; gap: 8px;">
                    <div class="badge-label">
                      <i class="fa-solid fa-calendar-days file-icon active"></i>
                      <span>Vigencia del Certificado</span>
                    </div>
                    <div class="validity-dates" style="font-size: 0.9rem; font-weight: 600; color: #475569; padding-left: 36px; display: flex; gap: 20px; flex-wrap: wrap;">
                      <span>Desde: <strong style="color: #0f172a;">{emisor.certificado_valido_desde}</strong></span>
                      <span>Hasta: <strong style="color: #0f172a;">{emisor.certificado_valido_hasta}</strong></span>
                    </div>
                  </div>
                {/if}
              </div>

              <div class="status-indicator configured">
                <i class="fa-solid fa-shield-halved"></i> Certificados CSD configurados y listos para firmar CFDI
              </div>
            </div>
          {:else}
            <!-- Form / Edit mode -->
            <form
              onsubmit={(e) => {
                e.preventDefault();
                saveCertificados();
              }}
              class="animate-fade-in"
            >
              <p class="form-instructions">
                Sube tus archivos CSD autorizados por el SAT para poder realizar el sellado digital de las facturas.
              </p>

              <div class="form-grid" style="margin-bottom: 20px;">
                <div class="form-group">
                  <label for="cer-upload">Archivo Certificado (.cer)</label>
                  <label 
                    class="file-upload-zone {isDraggingCer ? 'dragging' : ''}" 
                    for="cer-upload"
                    ondragover={(e) => e.preventDefault()}
                    ondragenter={() => (isDraggingCer = true)}
                    ondragleave={() => (isDraggingCer = false)}
                    ondrop={handleCerDrop}
                  >
                    <i class="fa-solid fa-file-contract file-icon {cerFile ? 'active' : ''}"></i>
                    <span class="file-info">
                      {cerFile ? cerFile.name : (emisor?.certificado_nombre || "Seleccionar archivo .cer")}
                    </span>
                    <input
                      id="cer-upload"
                      type="file"
                      accept=".cer"
                      onchange={handleCerChange}
                    />
                  </label>
                </div>

                <div class="form-group">
                  <label for="key-upload">Archivo Llave Privada (.key)</label>
                  <label 
                    class="file-upload-zone {isDraggingKey ? 'dragging' : ''}" 
                    for="key-upload"
                    ondragover={(e) => e.preventDefault()}
                    ondragenter={() => (isDraggingKey = true)}
                    ondragleave={() => (isDraggingKey = false)}
                    ondrop={handleKeyDrop}
                  >
                    <i class="fa-solid fa-key file-icon {keyFile ? 'active' : ''}"></i>
                    <span class="file-info">
                      {keyFile ? keyFile.name : (emisor?.llave_nombre || "Seleccionar archivo .key")}
                    </span>
                    <input
                      id="key-upload"
                      type="file"
                      accept=".key"
                      onchange={handleKeyChange}
                    />
                  </label>
                </div>

                <div class="form-group col-span-2">
                  <label for="csd-password">Contraseña de la Llave Privada</label>
                  <div class="password-input-container">
                    <input
                      id="csd-password"
                      type={showPassword ? "text" : "password"}
                      bind:value={passwordCSD}
                      placeholder="Contraseña del certificado CSD"
                      required
                      class="form-control"
                      style="width: 100%; padding-right: 40px;"
                    />
                    <button
                      type="button"
                      class="password-toggle-btn"
                      onclick={() => (showPassword = !showPassword)}
                      aria-label={showPassword ? "Ocultar contraseña" : "Mostrar contraseña"}
                    >
                      {#if showPassword}
                        <i class="fa-solid fa-eye-slash"></i>
                      {:else}
                        <i class="fa-solid fa-eye"></i>
                      {/if}
                    </button>
                  </div>
                </div>
              </div>

              <div class="form-actions">
                {#if emisor && emisor.certificado_nombre && showCsdForm}
                  <button
                    type="button"
                    class="btn btn-secondary"
                    onclick={() => {
                      showCsdForm = false;
                      cerFile = null;
                      keyFile = null;
                      csdErrorMessage = "";
                      passwordCSD = emisor?.llave_password || "";
                    }}
                  >
                    Cancelar
                  </button>
                {/if}
                <button type="submit" class="btn btn-primary" disabled={isUploadingCSD}>
                  {#if isUploadingCSD}
                    <i class="fa-solid fa-spinner fa-spin"></i> Subiendo...
                  {:else}
                    <i class="fa-solid fa-cloud-arrow-up"></i> Guardar Certificados
                  {/if}
                </button>
              </div>
            </form>
          {/if}
        </div>
      </div>
    </div>

    <!-- Side info / validation guide card -->
    <div class="info-card">
      <h3><i class="fa-solid fa-circle-info"></i> Guía SAT CFDI 4.0</h3>
      <p class="info-intro">
        Para la emisión exitosa de CFDI 4.0, el SAT valida de forma estricta los
        siguientes puntos:
      </p>

      <ul class="guide-list">
        <li>
          <strong>Nombre/Razón Social:</strong> Debe registrarse en MAYÚSCULAS y
          omitirse la palabra del régimen capital (ej. omitir
          <em>"S.A. DE C.V."</em>
          o <em>"S. DE R.L. DE C.V."</em>).
        </li>
        <li>
          <strong>Coincidencia de Datos:</strong> El RFC y la Razón Social deben
          coincidir de forma idéntica con el registro en la base de datos del SAT
          (Cédula de Identificación Fiscal).
        </li>
        <li>
          <strong>Régimen Fiscal:</strong> Asegúrate de que el régimen seleccionado
          coincida con las actividades económicas registradas por tu empresa ante
          el SAT.
        </li>
      </ul>

      <div class="cif-banner">
        <i class="fa-solid fa-qrcode qr-icon"></i>
        <div>
          <h4>¿Dudas con los datos?</h4>
          <p>
            Consulta la Constancia de Situación Fiscal de tu empresa para mayor
            precisión.
          </p>
        </div>
      </div>
    </div>
  </div>
</div>

<style>
  .container {
    max-width: 1200px;
    margin: 0 auto;
  }

  .page-header {
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

  /* Grid layout for two columns */
  .emisor-layout {
    display: grid;
    grid-template-columns: 1.6fr 1fr;
    gap: 28px;
    align-items: start;
  }

  @media (max-width: 900px) {
    .emisor-layout {
      grid-template-columns: 1fr;
    }
  }

  /* Main Card styling */
  .main-card {
    background: #fff;
    border-radius: 14px;
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.03);
    border: 1px solid #edf2f7;
    overflow: hidden;
  }

  .card-header {
    padding: 20px 24px;
    border-bottom: 1px solid #edf2f7;
    background: #f8fafc;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .card-header h2 {
    margin: 0;
    font-size: 1.25rem;
    font-weight: 700;
    color: #1e293b;
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .card-body {
    padding: 24px;
  }

  /* Display mode styling */
  .data-display {
    display: flex;
    flex-direction: column;
    gap: 20px;
  }

  .display-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
  }

  @media (max-width: 550px) {
    .display-grid {
      grid-template-columns: 1fr;
    }
  }

  .display-group {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .display-group .label {
    font-size: 0.8rem;
    font-weight: 700;
    color: #94a3b8;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .display-group .value {
    font-size: 1.05rem;
    color: #1e293b;
    font-weight: 600;
  }

  .display-group .value.company-name {
    font-size: 1.4rem;
    font-weight: 800;
    letter-spacing: -0.5px;
    color: #0f172a;
  }

  .display-group .value.code-font {
    font-family: monospace;
    font-size: 1.15rem;
  }

  .status-indicator {
    padding: 12px 16px;
    border-radius: 8px;
    font-weight: 600;
    font-size: 0.9rem;
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 10px;
  }

  .status-indicator.configured {
    background: #ecfdf5;
    color: #047857;
    border: 1px solid #a7f3d0;
  }

  /* Info/guideline card styling */
  .info-card {
    background: #edf2f7;
    border-radius: 14px;
    padding: 24px;
    border: 1px solid #e2e8f0;
    color: #2d3748;
  }

  .info-card h3 {
    margin: 0 0 12px 0;
    font-size: 1.15rem;
    font-weight: 700;
    color: #1a202c;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .info-intro {
    font-size: 0.9rem;
    line-height: 1.5;
    color: #4a5568;
    margin-bottom: 16px;
  }

  .guide-list {
    padding-left: 20px;
    margin: 0 0 24px 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
    font-size: 0.9rem;
    line-height: 1.5;
  }

  .guide-list li strong {
    color: #1a202c;
  }

  .cif-banner {
    background: #fff;
    border-radius: 10px;
    padding: 16px;
    border: 1px solid #cbd5e1;
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .qr-icon {
    font-size: 2.2rem;
    color: #4facfe;
  }

  .cif-banner h4 {
    margin: 0 0 4px 0;
    font-size: 0.95rem;
    font-weight: 700;
    color: #1a202c;
  }

  .cif-banner p {
    margin: 0;
    font-size: 0.85rem;
    color: #718096;
    line-height: 1.4;
  }

  /* Form styling */
  .form-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    margin-bottom: 24px;
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

  .form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    border-top: 1px solid #edf2f7;
    padding-top: 20px;
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

  .alert-success {
    background: #d1fae5;
    color: #065f46;
    border: 1px solid #a7f3d0;
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

  .animate-shake {
    animation: shake 0.35s ease-in-out;
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
    }
    to {
      opacity: 1;
    }
  }

  @keyframes shake {
    0%,
    100% {
      transform: translateX(0);
    }
    25% {
      transform: translateX(-4px);
    }
    75% {
      transform: translateX(4px);
    }
  }

  .main-column {
    display: flex;
    flex-direction: column;
    gap: 28px;
  }

  /* CSD card styling */
  .csd-card {
    background: #fff;
    border-radius: 14px;
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.03);
    border: 1px solid #edf2f7;
    overflow: hidden;
  }

  .form-instructions {
    font-size: 0.9rem;
    color: #64748b;
    margin-bottom: 20px;
    line-height: 1.5;
  }

  .file-upload-zone {
    border: 2px dashed #cbd5e1;
    border-radius: 10px;
    padding: 24px 16px;
    text-align: center;
    background: #f8fafc;
    cursor: pointer;
    transition: all 0.2s;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    margin-top: 4px;
  }

  .file-upload-zone:hover {
    border-color: #4facfe;
    background: #f0f9ff;
  }

  .file-upload-zone.dragging {
    border-color: #4facfe;
    background: #e0f2fe;
    transform: scale(1.02);
    box-shadow: 0 4px 12px rgba(79, 172, 254, 0.15);
  }

  .file-upload-zone input[type="file"] {
    display: none;
  }

  .file-info {
    font-size: 0.85rem;
    color: #475569;
    font-weight: 600;
    max-width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .file-icon {
    font-size: 2rem;
    color: #94a3b8;
    transition: color 0.2s;
  }

  .file-icon.active {
    color: #4facfe;
  }

  .csd-status-badges {
    display: flex;
    flex-direction: column;
    gap: 12px;
    margin-bottom: 20px;
  }

  .badge-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 14px 18px;
    border-radius: 8px;
    background: #f8fafc;
    border: 1px solid #edf2f7;
  }

  .badge-item .badge-label {
    display: flex;
    align-items: center;
    gap: 12px;
    font-weight: 600;
    color: #334155;
  }

  .badge-item .badge-value {
    font-size: 0.8rem;
    padding: 6px 12px;
    border-radius: 6px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .badge-value.loaded {
    background: #ecfdf5;
    color: #047857;
    border: 1px solid #a7f3d0;
  }

  .badge-value.pending {
    background: #fef2f2;
    color: #991b1b;
    border: 1px solid #fecaca;
  }

  .password-input-container {
    position: relative;
    display: flex;
    align-items: center;
    margin-top: 4px;
  }

  .password-toggle-btn {
    position: absolute;
    right: 12px;
    background: none;
    border: none;
    color: #64748b;
    cursor: pointer;
    padding: 8px;
    font-size: 1.1rem;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: color 0.2s;
  }

  .password-toggle-btn:hover {
    color: #0f172a;
  }

  .status-indicator-warning {
    padding: 12px 16px;
    border-radius: 8px;
    font-weight: 600;
    font-size: 0.9rem;
    display: flex;
    align-items: center;
    gap: 8px;
    background: #fffbe6;
    color: #ad7c11;
    border: 1px solid #ffe58f;
  }
</style>
