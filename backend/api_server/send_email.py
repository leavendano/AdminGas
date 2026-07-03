#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import argparse
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders

SMTP_USER = "cfdi@infinitummail.com"
SMTP_PASS = "Dnb49725"
SMTP_HOST = "smtp.infinitummail.com"
SMTP_PORT = 465

def send_cfdi_email(email_to, xml_path, pdf_path, uuid):
    # Verify files exist
    if not os.path.exists(xml_path):
        raise FileNotFoundError(f"XML file not found: {xml_path}")
    if not os.path.exists(pdf_path):
        raise FileNotFoundError(f"PDF file not found: {pdf_path}")

    # Build email message
    msg = MIMEMultipart()
    msg['From'] = SMTP_USER
    msg['To'] = email_to
    msg['Subject'] = f"Comprobante Fiscal Digital por Internet (CFDI) - Folio Fiscal: {uuid}"

    # Body text
    body = f"Estimado cliente,\n\n" \
           f"Se adjunta a este correo el Comprobante Fiscal Digital por Internet (CFDI) " \
           f"correspondiente a su facturación, en formatos XML y PDF.\n\n" \
           f"Folio Fiscal (UUID): {uuid}\n\n" \
           f"Atentamente,\n" \
           f"Infosystems"
           
    msg.attach(MIMEText(body, 'plain'))

    # Helper function to attach files
    def attach_file(path, filename):
        with open(path, 'rb') as f:
            part = MIMEBase('application', 'octet-stream')
            part.set_payload(f.read())
            encoders.encode_base64(part)
            part.add_header(
                'Content-Disposition',
                f'attachment; filename="{filename}"'
            )
            msg.attach(part)

    # Attach XML and PDF
    attach_file(xml_path, f"CFDI_{uuid}.xml")
    attach_file(pdf_path, f"CFDI_{uuid}.pdf")

    # Connect to server and send
    print(f"Connecting to SMTP server {SMTP_HOST}:{SMTP_PORT} via SSL...")
    server = smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT, timeout=20)
    
    try:
        print("Logging in...")
        server.login(SMTP_USER, SMTP_PASS)
        print(f"Sending email to {email_to}...")
        server.sendmail(SMTP_USER, email_to, msg.as_string())
        print("Email sent successfully!")
    finally:
        server.quit()

def main():
    parser = argparse.ArgumentParser(description="Send CFDI XML and PDF via SMTP email.")
    parser.add_argument('--email', required=True, help="Recipient email address")
    parser.add_argument('--xml', required=True, help="Path to the XML file")
    parser.add_argument('--pdf', required=True, help="Path to the PDF file")
    parser.add_argument('--uuid', required=True, help="UUID of the CFDI invoice")
    
    args = parser.parse_args()
    
    try:
        send_cfdi_email(args.email, args.xml, args.pdf, args.uuid)
        sys.exit(0)
    except Exception as e:
        print(f"SMTP Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
