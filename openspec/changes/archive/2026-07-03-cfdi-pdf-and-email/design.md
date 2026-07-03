## Context

The current invoicing system compiles C++ code using Drogon. While Drogon is highly efficient at handling HTTP endpoints and ORM persistence, generating styled PDF reports directly in C++ is complex and lacks flexible layout engines. Since the system already has Python 3 and the `reportlab` library installed, we can delegate the PDF rendering and SMTP email sending to simple, self-contained Python scripts invoked as subprocesses from the C++ controllers.

## Goals / Non-Goals

**Goals:**
- Dynamically generate a SAT-compliant visual PDF representation of a CFDI 4.0 under `/factura/{id}/pdf`.
- Support emailing both the XML and dynamically generated PDF of certified invoices to the recipient under `/factura/{id}/email`.
- Add an email address field to the `receptor` table, model, and forms.
- Expose visual options in the front-end to trigger download and email actions for certified invoices.

**Non-Goals (Qué NO hay que tocar):**
- **NO modificar** the core XML signing and PAC stamping implementation logic inside [CfdiHelper.cc](file:///home/lavenda/Projects/AdminGas/backend/api_server/controllers/CfdiHelper.cc) or [CfdiHelper.h](file:///home/lavenda/Projects/AdminGas/backend/api_server/controllers/CfdiHelper.h) (e.g., `generarXmlProvisional`, `generarCadenaOriginal`, `generarSello`, `generarXmlFirmado`, `enviarAFinkok`). We must only read the completed `xml_timbrado` from the database.
- **NO modificar** the authentication system or endpoints inside [AuthController.cc](file:///home/lavenda/Projects/AdminGas/backend/api_server/controllers/AuthController.cc) or [AuthController.h](file:///home/lavenda/Projects/AdminGas/backend/api_server/controllers/AuthController.h).
- **NO modificar** the Todo list schema, logic, or templates.
- **NO almacenar** generated PDF files persistently on the server's disk. Any PDF created must be temporary, read immediately, and immediately cleaned up.

## Decisions

### Decision 1: PDF Generation Engine
- **Choice**: Python 3 Subprocess + ReportLab.
- **Rationale**: ReportLab is already installed and provides a high-level API to build styled grids, headers, paragraph text formatting, and native ISO-compliant QR code widgets. Writing PDF generation directly in C++ would require compiling external libraries (e.g., `libharu`) and manually managing coordinates, text line heights, and complex text wrapping, which is highly error-prone.
- **Alternatives Considered**: 
  - Using a C++ PDF library: Rejected due to compilation overhead and lack of robust layout/text-wrapping utilities.
  - Using node module/headless chrome: Headless chrome or Puppeteer are not installed and have massive resource requirements.

### Decision 2: Process Communication via Temp Files
- **Choice**: C++ writes the DB's `xml_timbrado` to a unique temp XML file in the `uploads` directory, calls `python3 cfdi_to_pdf.py <xml_path> <pdf_path>`, reads the output PDF file bytes, and deletes both files.
- **Rationale**: Simple, highly robust, and avoids shell escaping issues when passing huge XML strings directly through terminal parameters.
- **Alternatives Considered**: Passing XML data directly via standard input pipes: Rejected as it introduces complex multi-directional pipe handling and buffer block risks in C++.

### Decision 3: Email Delivery Mechanism
- **Choice**: Subprocess execution of `send_email.py` using Python's built-in `smtplib` over SSL (port 465).
- **Rationale**: Python provides a clean `email.mime` package to attach multiple files (XML and PDF) with proper headers. Implementing secure SMTP with SSL and attachments in C++ would require writing SMTP protocols from scratch and manually compiling/linking with SSL socket libraries.
- **Alternatives Considered**: Direct C++ SMTP library: Rejected due to complexity, lack of standard library support, and security maintenance.

## Risks / Trade-offs

- **[Risk]**: Performance overhead when starting a Python subprocess.
  - **Mitigation**: Spawning Python processes is only done on-demand (when a user explicitly downloads a PDF or requests an email). This is not on critical real-time execution paths, and the C++ event loop will handle concurrent connections safely.
- **[Risk]**: SMTP credentials security.
  - **Mitigation**: Credentials (`cfdi@infinitummail.com` and password `Dnb49725`) are defined as constant configurations in the python script. Since the python script is executed locally by the server, these credentials are kept private.
- **[Risk]**: SMTP sending errors (e.g., timeouts, wrong email format).
  - **Mitigation**: The python script will exit with a non-zero code on failure. The C++ backend will capture the error output from the script and return a clear `500 Internal Server Error` containing the error message to the frontend, notifying the user.
