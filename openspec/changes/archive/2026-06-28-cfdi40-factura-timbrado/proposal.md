## Why

Implementar el cumplimiento fiscal mexicano (CFDI 4.0) permitiendo a la plataforma AdminGas emitir facturas electrónicas válidas, selladas digitalmente a través de la clave privada del emisor (CSD) y timbradas mediante la conexión a un Proveedor Autorizado de Certificación (PAC) real de pruebas (Finkok). Esto permite dar soporte a la facturación electrónica mexicana dentro del dashboard administrativo de la aplicación.

## What Changes

- **Base de datos (PostgreSQL)**: Extensión de la tabla `factura` para almacenar el XML timbrado final (`xml_timbrado`), el UUID fiscal (`uuid`), el sello digital (`sello`) y el estado de la factura (`status` con valor inicial 'creada').
- **Modelos C++ (Drogon ORM)**: Regeneración del modelo de `Factura` para reflejar las nuevas columnas mediante `drogon_ctl`.
- **Serialización CFDI 4.0**: Mapeo y generación del documento XML estructurado según el estándar CFDI 4.0 utilizando las clases C++ generadas por CodeSynthesis XSD (`xsd/generated/cfdv40.h`).
- **Cadena Original**: Integración nativa con `libxslt` para aplicar la transformación del stylesheet oficial `cadenaoriginal_4_0.xslt` y generar la cadena original.
- **Firma y Sello Digital**: Carga de la clave privada (llave) del emisor y firma criptográfica SHA-256 con RSA usando la biblioteca OpenSSL integrada.
- **Integración con PAC Finkok**: Conexión al servicio de pruebas (Demo) del PAC Finkok mediante peticiones HTTP POST con SOAP envelopes. El sistema empaquetará el XML firmado en Base64 y procesará la respuesta de Finkok para obtener el XML timbrado definitivo y el UUID asignado.
- **API Endpoints**: 
  - `POST /factura/{id}/timbrar`: Endpoint que ejecuta el flujo completo de timbrado.
  - `GET /factura/{id}/xml`: Endpoint para descargar el XML timbrado resultante.
- **Frontend (Svelte 5)**: Botón interactivo de "Timbrar CFDI" en la lista de facturas, visualización del estado/UUID y descarga directa del XML.

## Capabilities

### New Capabilities
- `factura-cfdi-timbrado`: Proceso completo de serialización de la entidad Factura al XML del CFDI 4.0, extracción de cadena original, firmado por CSD del emisor, timbrado a través del servicio SOAP de Finkok, e integración con la interfaz de usuario en el frontend para el control y descarga de XML.

### Modified Capabilities
<!-- No requirement changes in existing features. -->

## Impact

- **C++ Backend**:
  - `CMakeLists.txt`: Configuración de dependencias para `libxml2`, `libxslt`, `libexslt` y `libxerces-c`.
  - `main.cc`: Inicialización y apagado de Xerces-C++ (`xercesc::XMLPlatformUtils`).
  - `controllers/FacturaController.h` & `controllers/FacturaController.cc`: Nuevos endpoints y lógica de negocios del CFDI.
  - `models/Factura.*`: Regenerados mediante `drogon_ctl`.
  - `db_schema.sql`: Sentencias actualizadas de datos.
- **Frontend**:
  - `frontend/src/routes/facturas/+page.svelte`: Manejo interactivo de estados e integración de los nuevos endpoints del backend.
- **Dependencias**:
  - Requiere la instalación del paquete `libxerces-c-dev` en el entorno de desarrollo.
