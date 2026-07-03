## MODIFIED Requirements

### Requirement: Interfaz de Usuario para Timbrado
La interfaz de usuario del frontend en Svelte 5 MUST permitir iniciar el proceso de timbrado, descargar el XML, descargar el PDF y enviar por correo las facturas timbradas.

#### Scenario: Timbrar factura desde la interfaz
- **WHEN** el usuario hace clic en el botón "Timbrar" para una factura en estado "creada"
- **THEN** el sistema realiza la petición `POST /factura/{id}/timbrar`, actualiza el estado en la tabla visual a "Timbrada" y habilita la descarga de su XML, la descarga de su PDF y la opción de envío por correo.
