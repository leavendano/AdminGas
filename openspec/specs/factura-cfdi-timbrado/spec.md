# Spec: Facturación CFDI 4.0 y Timbrado

Esta especificación describe los requerimientos para la generación de facturas XML en formato CFDI 4.0, la obtención de la firma digital (sello) y su certificación (timbrado) mediante el PAC Finkok.

## Requirements

### Requirement: Serialización XML de Factura CFDI 4.0
El sistema MUST serializar la información de la entidad Factura (emisor, receptor y conceptos) en un archivo XML estructurado cumpliendo con el estándar CFDI 4.0 definido por el SAT.

#### Scenario: Generación exitosa de XML provisional
- **WHEN** el usuario solicita timbrar una factura de ingreso
- **THEN** el sistema genera un XML provisional conforme al esquema `cfdv40.xsd` con los datos del emisor, receptor, conceptos, impuestos y subtotales/totales calculados, pero con el atributo Sello y Certificado vacíos.

### Requirement: Extracción de la Cadena Original
El sistema MUST aplicar el stylesheet oficial `cadenaoriginal_4_0.xslt` mediante `libxslt` al XML provisional para generar la Cadena Original.

#### Scenario: Transformación exitosa mediante libxslt
- **WHEN** se cuenta con el XML provisional de la factura en memoria
- **THEN** el sistema utiliza la biblioteca `libxslt` en C++ para procesar la hoja de estilo XSLT y retorna el string de la Cadena Original limpia y formateada.

### Requirement: Generación del Sello Digital (Firma)
El sistema MUST firmar la Cadena Original del CFDI con la clave privada del emisor usando el algoritmo SHA-256 con RSA y adjuntar el sello resultante codificado en Base64 al XML del comprobante.

#### Scenario: Firmado y sellado del CFDI
- **WHEN** el sistema cuenta con la Cadena Original, la clave privada del emisor (columna `llave` de `emisor`) decodificada y descifrada con su contraseña (`llave_password`), y el número de certificado
- **THEN** el sistema genera la firma SHA-256+RSA, la codifica en Base64, e inserta los atributos `Sello`, `NoCertificado` y `Certificado` en la raíz del XML.

### Requirement: Timbrado mediante PAC Finkok
El sistema MUST enviar el XML firmado al servicio web SOAP de Finkok para su certificación y timbrado.

#### Scenario: Timbrado exitoso con Finkok
- **WHEN** el sistema envía el XML firmado y codificado en Base64 en un envelope SOAP al endpoint de Finkok de pruebas con las credenciales configuradas
- **THEN** Finkok retorna una respuesta SOAP exitosa que contiene el XML completo timbrado (con su nodo `tfd:TimbreFiscalDigital` v1.1) y el UUID en el tag `<s0:xml>` y `<s0:UUID>`.

#### Scenario: Error de validación o timbrado rechazado por Finkok
- **WHEN** el XML firmado contiene errores de validación fiscal (p. ej. RFC inexistente, fecha fuera de rango o sello inválido) y se envía a Finkok
- **THEN** Finkok retorna un mensaje SOAP con incidencias en el nodo `<s0:Incidencias>`, y el sistema extrae el mensaje de error y retorna un código HTTP 400 Bad Request al cliente con la descripción del error.

### Requirement: Persistencia del XML y Metadatos
El sistema MUST guardar el XML timbrado final y el UUID fiscal en la base de datos de la factura, y actualizar el estatus de la factura a 'timbrada'.

#### Scenario: Guardado de datos de timbrado
- **WHEN** Finkok retorna el XML timbrado final con éxito
- **THEN** el sistema actualiza el registro en la base de datos PostgreSQL asignándole el XML timbrado al campo `xml_timbrado`, el UUID al campo `uuid`, la firma al campo `sello` y establece el estado a 'timbrada'.

### Requirement: Descarga del XML del CFDI
El sistema MUST permitir la descarga del archivo XML timbrado correspondiente a una factura.

#### Scenario: Descarga exitosa de XML
- **WHEN** el cliente realiza una petición HTTP `GET` a la ruta `/factura/{id}/xml` para una factura con estatus 'timbrada'
- **THEN** el sistema responde con el contenido del XML timbrado y establece las cabeceras HTTP `Content-Type: application/xml`.

### Requirement: Interfaz de Usuario para Timbrado
La interfaz de usuario del frontend en Svelte 5 MUST permitir iniciar el proceso de timbrado y descargar el XML para las facturas registradas.

#### Scenario: Timbrar factura desde la interfaz
- **WHEN** el usuario hace clic en el botón "Timbrar" para una factura en estado "creada"
- **THEN** el sistema realiza la petición `POST /factura/{id}/timbrar`, actualiza el estado en la tabla visual a "Timbrada" y habilita la descarga de su XML.
