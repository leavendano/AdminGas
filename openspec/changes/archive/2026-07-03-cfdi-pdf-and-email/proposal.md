## Why

Currently, the system only supports generating and downloading the XML representation of a certified CFDI 4.0 invoice. In order to provide a complete billing solution, users must be able to download a human-readable visual PDF representation of the invoice (matching the layout of the `J0297.pdf` template) and directly email both the XML and PDF files to the client's registered email address.

## What Changes

- **Database Schema Modification**:
  - Add a nullable `email` column of type `VARCHAR(255)` to the `receptor` table to store the customer's email address.
- **Backend Model Update**:
  - Regenerate the Drogon ORM models for `Receptor` using `drogon_ctl` to support the new `email` field.
- **New Backend API Endpoints**:
  - `GET /factura/{id}/pdf`: Dynamically generates the PDF representation from the invoice's `xml_timbrado` data and streams the PDF back to the client as a download attachment. No PDF files are persistently stored on the server.
  - `POST /factura/{id}/email`: Dynamically generates the PDF, and sends an email containing both the XML and PDF as attachments to the receptor's email address using the infinitummail SMTP settings. Allows overriding the destination email via the JSON request body.
- **Helper Scripts**:
  - `cfdi_to_pdf.py`: Python script utilizing ReportLab to parse the CFDI 4.0 XML and draw a premium, professional PDF conforming to SAT regulations and the layout guidelines of `J0297.pdf`, including an standard-compliant ISO/IEC18004 QR code.
  - `send_email.py`: Python script utilizing `smtplib` to send emails with attachments via the configured SSL SMTP server.
- **Frontend Enhancements**:
  - `receptores/+page.svelte`: Add an "Email" field in the customer creation/edit modal and display it under the customer's name in the table.
  - `facturas/+page.svelte`: Add two new buttons for each certified ("timbrada") invoice in the table: "Descargar PDF" (red PDF icon) and "Enviar por Correo" (purple paper plane icon). Shows a spinner when sending the email, followed by a success/error message.

## Capabilities

### New Capabilities
- `cfdi-pdf-and-email`: Covers the dynamic visual PDF generation of certified invoices and their email transmission containing both XML and PDF documents.

### Modified Capabilities
- `factura-cfdi-timbrado`: Modified to extend the certified invoice options from simple XML download to visual PDF download and email delivery.

## Impact

- **Database**:
  - `receptor` table schema (adds `email` column).
- **Backend (C++)**:
  - `models/Receptor.h` and `models/Receptor.cc` (regenerated).
  - `controllers/FacturaController.h` and `controllers/FacturaController.cc` (adds getPdf and sendEmail endpoints).
- **Backend (Python)**:
  - New utility scripts `cfdi_to_pdf.py` and `send_email.py`.
- **Frontend (SvelteKit)**:
  - `src/routes/receptores/+page.svelte` (form input, interfaces, table details).
  - `src/routes/facturas/+page.svelte` (download links, sending actions, state management, buttons, icons, table styling).
