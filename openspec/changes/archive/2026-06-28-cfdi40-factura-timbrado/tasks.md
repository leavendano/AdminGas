## 1. Setup y Preparación del Entorno

- [x] 1.1 Instalar el paquete de desarrollo `libxerces-c-dev` en el sistema operativo mediante el gestor de paquetes.
- [x] 1.2 Modificar el archivo `db_schema.sql` para añadir las sentencias `ALTER TABLE` necesarias que incorporen las nuevas columnas `xml_timbrado` (TEXT), `uuid` (VARCHAR(36)), `sello` (VARCHAR(500)) y `status` (VARCHAR(20)) a la tabla `factura`.
- [x] 1.3 Aplicar de forma manual las sentencias SQL `ALTER TABLE` sobre la base de datos PostgreSQL local `admingas` usando el usuario `postgres` y la contraseña `admingas_password`.
- [x] 1.4 Ejecutar el script `update_model.py` para asegurar que las credenciales de la DB sean correctas en `model.json`, y correr el comando de generación `drogon_ctl create_model models` en la carpeta `backend/api_server/` para regenerar la clase `Factura` con los nuevos getters y setters del ORM.
- [x] 1.5 Modificar `CMakeLists.txt` del backend para localizar los paquetes `LibXml2`, `libxslt`, `libexslt` (con PkgConfig) y `Xerces-C`, añadir la ruta de cabeceras de `xsd/generated`, incluir `xsd/generated/cfdv40.cpp`, `tdCFDI.cpp`, y `catCFDI.cpp` en las fuentes del target `api_server`, y enlazarlos al ejecutable.
- [x] 1.6 Modificar `main.cc` para incluir `<xercesc/util/PlatformUtils.hpp>` y registrar las llamadas de inicialización y finalización `xercesc::XMLPlatformUtils::Initialize()` y `Terminate()`.

## 2. Lógica Core y Utilerías de CFDI 4.0

- [x] 2.1 Crear los archivos de utilerías del CFDI `CfdiHelper.h` y `CfdiHelper.cc` dentro de `backend/api_server/controllers/` o un subdirectorio compartido.
- [x] 2.2 Implementar en `CfdiHelper` la función para instanciar y estructurar el objeto `cfdi::Comprobante` (con su Emisor, Receptor, Conceptos, Impuestos y subtotales/totales calculados) a partir de un registro de `Factura` y serializarlo a un XML provisional en memoria.
- [x] 2.3 Implementar en `CfdiHelper` la función para aplicar la transformación XSLT con `cadenaoriginal_4_0.xslt` usando la biblioteca `libxslt` en C++ y retornar la Cadena Original resultante.
- [x] 2.4 Implementar en `CfdiHelper` la función que cargue la llave privada DER (PKCS#8) y el certificado del emisor (Base64) usando OpenSSL, los descifre con la contraseña y firme la cadena original con SHA-256 + RSA para retornar el Sello en Base64.
- [x] 2.5 Implementar en `CfdiHelper` la inyección de los atributos `Sello`, `NoCertificado` y `Certificado` en el `cfdi::Comprobante` y generar el XML firmado final.
- [x] 2.6 Desarrollar en `CfdiHelper` la comunicación con Finkok: construir el SOAP Envelope, realizar el POST HTTP asíncrono con `drogon::HttpClient` al servicio de pruebas de Finkok, parsear el XML de la respuesta con `libxml2` para extraer el CFDI timbrado final, su UUID o extraer las incidencias de error del PAC.

## 3. Controladores y API Endpoints

- [x] 3.1 Registrar en `FacturaController.h` y `FacturaController.cc` los nuevos métodos `timbrar` (POST en `/factura/{id}/timbrar`) y `getXml` (GET en `/factura/{id}/xml`).
- [x] 3.2 Implementar la lógica del método `FacturaController::timbrar` para orquestar la obtención de la factura, el emisor y el receptor desde la base de datos, ejecutar el flujo del helper `CfdiHelper`, guardar el XML final y metadatos en la DB, y actualizar el estatus a "timbrada".
- [x] 3.3 Implementar el método `FacturaController::getXml` para recuperar el XML timbrado de la base de datos de la factura con el ID correspondiente y responder con las cabeceras HTTP `Content-Type: application/xml`.

## 4. Interfaz de Usuario Frontend

- [x] 4.1 Modificar la tabla de facturas en `frontend/src/routes/facturas/+page.svelte` para mostrar una columna o badge de Estado ("Creada" o "Timbrada") y desplegar el UUID de la factura timbrada.
- [x] 4.2 Agregar las acciones "Timbrar CFDI" y "Descargar XML" a la columna de Acciones en la tabla de facturas dependiendo de su estado de timbrado.
- [x] 4.3 Implementar la llamada asíncrona `stampFactura(id)` en la sección `<script>` de `+page.svelte` para interactuar con el endpoint `/factura/{id}/timbrar`, mostrar loaders durante el timbrado, recargar reactivamente la lista de facturas y mostrar mensajes Toast de éxito o error.
- [x] 4.4 Agregar reglas de CSS específicas en el bloque `<style>` de `+page.svelte` para estilizar los badges de estado del CFDI y los nuevos botones de acción.
