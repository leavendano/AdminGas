# Spec: Generación de PDF y Envío de Email de CFDI

## Purpose
Esta especificación define los requerimientos para la generación dinámica de la representación impresa en PDF de un CFDI 4.0 y para posibilitar el envío por correo electrónico de los comprobantes XML y PDF.

## Requirements

## ADDED Requirements

### Requirement: Campo de Email del Receptor
El sistema MUST almacenar una dirección de correo electrónico para cada receptor con el fin de posibilitar el envío automático de comprobantes CFDI.

#### Scenario: Registro o edición exitosa de email del receptor
- **WHEN** el usuario registra o actualiza un receptor proporcionando una dirección de correo electrónico válida
- **THEN** el sistema persiste dicho correo en la base de datos y lo asocia con el receptor correspondiente.

### Requirement: Generación Dinámica de Representación Impresa en PDF
El sistema MUST ser capaz de generar dinámicamente, y sin almacenamiento permanente en disco, la representación impresa en formato PDF de un CFDI 4.0 timbrado, conteniendo la información de cabeceras, receptor, emisor, conceptos, totales con letra y sellos digitales, incluyendo un código QR conforme a la norma ISO/IEC18004.

#### Scenario: Petición exitosa de descarga del PDF de una factura
- **WHEN** el cliente solicita la descarga del PDF mediante una petición HTTP `GET` a `/factura/{id}/pdf` para una factura con estatus 'timbrada'
- **THEN** el sistema procesa el XML timbrado guardado en la base de datos, genera dinámicamente el PDF con el diseño correspondiente al formato `J0297.pdf` y lo retorna al cliente con la cabecera `Content-Type: application/pdf`.

### Requirement: Envío de CFDI por Correo Electrónico
El sistema MUST permitir el envío por correo electrónico de los archivos XML y PDF de una factura timbrada utilizando un servidor SMTP seguro con autenticación SSL.

#### Scenario: Envío de correo exitoso al correo del receptor
- **WHEN** el cliente realiza una petición HTTP `POST` a `/factura/{id}/email` y el receptor tiene un correo configurado (o se especifica uno en el cuerpo de la petición)
- **THEN** el sistema genera el PDF dinámicamente, construye un correo con el XML timbrado y el PDF adjuntos, lo envía a través del servidor SMTP de Infinitum Mail, y retorna una respuesta de éxito.
