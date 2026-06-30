## Context

Actualmente, el sistema AdminGas permite crear facturas administrativas básicas (Emisor, Receptor, Conceptos, Totales) y persistirlas en una base de datos PostgreSQL. Sin embargo, carece de la capacidad de generar un Comprobante Fiscal Digital por Internet (CFDI 4.0) válido conforme a las leyes fiscales de México. La facturación oficial requiere la generación de una estructura XML (basada en el esquema `cfdv40.xsd`), su sellado digital mediante la firma de su cadena original con el certificado digital del emisor (CSD), y su timbrado oficial ante un PAC (Proveedor Autorizado de Certificación).

Este diseño introduce la integración con el servicio web de pruebas (Demo) del PAC Finkok para completar la serialización, el sellado y el timbrado de facturas de ingresos en formato CFDI 4.0, facilitando su posterior descarga y visualización desde el panel administrativo del frontend en Svelte 5.

## Goals / Non-Goals

**Goals:**
- Diseñar e implementar la serialización de la factura en XML bajo el esquema `cfdv40.xsd` empleando CodeSynthesis XSD en el backend C++.
- Implementar la generación nativa de la Cadena Original cargando y aplicando el stylesheet oficial `cadenaoriginal_4_0.xslt` usando la biblioteca `libxslt`.
- Integrar la lógica criptográfica para cargar la llave privada (`.key` cifrado DER PKCS#8) y el certificado del emisor y firmar el CFDI con SHA-256 + RSA usando OpenSSL para obtener el sello digital del comprobante.
- Desarrollar la integración SOAP con el PAC Finkok en el entorno de pruebas para obtener el Timbre Fiscal Digital (TFD v1.1) e insertarlo en el CFDI final.
- Crear los nuevos endpoints `POST /factura/{id}/timbrar` and `GET /factura/{id}/xml` en `FacturaController`.
- Agregar componentes interactivos en el frontend (Svelte 5) para que el usuario pueda visualizar el estado de timbrado, ejecutar el timbrado de facturas registradas y descargar su XML.

**Non-Goals (Qué NO hay que tocar):**
- **NO modificar la lógica existente del resto de endpoints del backend**: No se alterarán los controladores ni APIs CRUD de `TodoController`, `EmisorController`, `ReceptorController`, ni `ConceptoController`.
- **NO alterar el esquema de entrada ni de validación de Emisor, Receptor y Concepto**: El guardado, la estructura base y la validación de estos modelos existentes no se alterará.
- **NO alterar los métodos existentes de creación o eliminación en `FacturaController`**: El flujo actual para guardar facturas administrativas (`POST /factura`) y eliminarlas (`DELETE /factura/{id}`) no se debe modificar. El timbrado será un paso posterior que actúa como mutador asíncrono e independiente.
- **NO realizar timbrado comercial en producción**: Toda la lógica de timbrado se conectará al servicio de pruebas (Demo) del PAC Finkok. Las credenciales de producción no se utilizarán ni se almacenarán en esta fase.
- **NO implementar el envío de correos electrónicos ni representación impresa (PDF)**: La generación de archivos PDF o el envío automatizado de correos electrónicos al receptor queda fuera de este alcance.

## Decisions

- **Uso de `libxml2` y `libxslt` nativo en C++ para la transformación XSLT**:
  - *Decisión*: Integrar directamente las librerías `libxml2` y `libxslt` en el backend para aplicar el stylesheet `cadenaoriginal_4_0.xslt` en memoria.
  - *Razón*: El sistema ya tiene las bibliotecas y cabeceras de desarrollo instaladas en el entorno local (verificado en la fase de exploración). Esto evita levantar subprocesos del sistema (como `xsltproc` vía `fork`/`exec`), lo cual añade overhead y riesgos de seguridad.
  - *Alternativas consideradas*: Ejecutar `xsltproc` mediante la API de Drogon o `std::system()`. Descartado por ser ineficiente y propenso a inyección de comandos o problemas de rutas en runtime.
- **Modificación del esquema físico y regeneración del modelo de Drogon (`Factura`)**:
  - *Decisión*: Ejecutar sentencias SQL `ALTER TABLE` sobre la tabla `factura` para agregar campos de timbrado (`xml_timbrado`, `uuid`, `sello`, `status`) y regenerar la clase C++ usando `drogon_ctl create_model models`.
  - *Razón*: Almacenar estos valores en la base de datos permite responder solicitudes de descarga del XML de manera instantánea y persistente. La regeneración del modelo con `drogon_ctl` asegura el tipado seguro y mapeo nativo en Drogon, minimizando errores al interactuar con PostgreSQL.
  - *Alternativas consideradas*: Almacenar los XML en archivos locales en disco. Descartado por la complejidad de mantener sincronizado el almacenamiento en disco con los IDs de la base de datos, y por dificultades para entornos con múltiples instancias o contenedores.
- **Carga directa en memoria de la llave privada DER PKCS#8**:
  - *Decisión*: Leer el base64 de la clave privada (`llave`) guardado en la base de datos, decodificarlo a binario (DER) y usar `d2i_PKCS8PrivateKey_bio` de OpenSSL pasándole la contraseña (`llave_password`) para inicializar el objeto `EVP_PKEY*`.
  - *Razón*: Los archivos `.key` entregados por el SAT están en formato DER cifrado PKCS#8. Esta API de OpenSSL maneja el descifrado y la carga en memoria de manera segura y directa sin requerir convertir la llave a formato PEM ni escribir archivos temporales en el sistema.
  - *Alternativas consideradas*: Requerir que el usuario suba la clave privada ya convertida a formato PEM sin contraseña. Descartado porque rompe la experiencia de usuario y el flujo estándar de carga de CSDs del SAT.
- **Integración SOAP con Finkok (Pruebas)**:
  - *Decisión*: Consumir el Web Service de pruebas (Demo) de Finkok (`https://demo-facturacion.finkok.com/servicios/soap/stamp`) mediante `drogon::HttpClient`. Se enviará un Envelope SOAP conteniendo el XML firmado codificado en Base64, el username `leavendano@gmail.com` y la contraseña `W7y1g5z*`.
  - *Razón*: Permite certificar el CFDI contra un PAC real en su ambiente de pruebas de forma asíncrona (gracias a Drogon) y sin bloquear el hilo principal de la API. La respuesta XML se parseará con `libxml2` para extraer el CFDI timbrado con su nodo de timbre fiscal digital (`tfd:TimbreFiscalDigital` v1.1) y su UUID, o para extraer y propagar mensajes de incidencia en caso de rechazo del SAT/PAC.
  - *Alternativas consideradas*: Usar librerías de cliente SOAP pesadas como gSOAP. Descartadas por la gran cantidad de código que introducen, la inestabilidad con C++ moderno y el costo de configuración; dado que es un único request SOAP sencillo, construir el string XML a mano y enviar vía POST HTTP con `HttpClient` de Drogon es extremadamente simple, limpio y rápido.

## Risks / Trade-offs

- **[Riesgo] Dependencia de cabeceras de Xerces-C++**:
  - *Mitigación*: CodeSynthesis XSD genera wrappers C++ sobre Xerces-C++. Dado que `libxerces-c-dev` no está instalado en el sistema de desarrollo, debemos incluir explícitamente en el plan de tareas que el usuario instale el paquete (`sudo apt-get install libxerces-c-dev`) antes de compilar.
- **[Riesgo] Formato de fecha del SAT**:
  - *Mitigación*: El SAT requiere que el atributo `Fecha` del comprobante coincida con la zona horaria del emisor en formato `YYYY-MM-DDThh:mm:ss` y no tenga una diferencia mayor a 72 horas con el momento de timbrado. El controlador formateará la fecha del comprobante en tiempo de timbrado utilizando la fecha local del sistema ajustada de forma correcta.
- **[Riesgo] Dependencia de rutas relativas de XSLT**:
  - *Mitigación*: Para aplicar la transformación XSLT se requiere que `cadenaoriginal_4_0.xslt` y `utilerias.xslt` estén en una ubicación accesible. Usaremos una ruta configurable o una ruta relativa resuelta a partir del directorio del servidor de Drogon, garantizando que el servidor cargue las hojas de estilo de manera consistente.

