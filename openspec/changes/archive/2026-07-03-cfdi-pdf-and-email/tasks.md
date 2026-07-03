## 1. Database and Model Setup

- [ ] 1.1 Verify the postgres database table `receptor` has the new column `email` and the C++ models are successfully updated with the `email` fields.

## 2. Python Utility Scripts

- [ ] 2.1 Create python script `cfdi_to_pdf.py` in `backend/api_server` that parses a CFDI 4.0 XML and generates a styled PDF matching the `J0297.pdf` layout using ReportLab and includes an standard-compliant ISO/IEC18004 QR code.
- [ ] 2.2 Create python script `send_email.py` in `backend/api_server` that uses SMTP_SSL (port 465) with credentials `cfdi@infinitummail.com` and password `Dnb49725` to send emails with the XML and PDF files as attachments.

## 3. C++ Backend API Development

- [ ] 3.1 Register and implement the `GET /factura/{id}/pdf` endpoint in `FacturaController` to dynamically generate the PDF representation and return it as a download stream.
- [ ] 3.2 Register and implement the `POST /factura/{id}/email` endpoint in `FacturaController` that triggers `send_email.py` with dynamic files and handles custom body override emails.

## 4. Frontend Development

- [ ] 4.1 Update `src/routes/receptores/+page.svelte` to include the `email` field in Receptor interfaces, add/edit form modals, and show the email in the details list under the customer name.
- [ ] 4.2 Update `src/routes/facturas/+page.svelte` to support the updated Receptor interface, render "Descargar PDF" and "Enviar por Correo" buttons for timbradas facturas, manage loading states, and handle notifications.

## 5. Verification & Testing

- [ ] 5.1 Verify C++ api_server compiles successfully.
- [ ] 5.2 Verify Svelte frontend compiles and typescript checks pass.
- [ ] 5.3 Verify PDF generation matches standard SAT requirements and visually aligns with `J0297.pdf`.
- [ ] 5.4 Verify SMTP email sending succeeds and correctly delivers both attachments.
