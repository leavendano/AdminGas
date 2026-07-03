#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import xml.etree.ElementTree as ET
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.units import inch
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, KeepTogether
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.graphics.shapes import Drawing
from reportlab.graphics.barcode.qr import QrCodeWidget

# Catalog Mappings for CFDI
REGIMENES = {
    '601': '601 - General de Ley Personas Morales',
    '603': '603 - Personas Morales con Fines no Lucrativos',
    '605': '605 - Sueldos y Salarios e Ingresos Asimilados a Salarios',
    '606': '606 - Arrendamiento',
    '608': '608 - Demás ingresos',
    '612': '612 - Personas Físicas con Actividades Empresariales y Profesionales',
    '621': '621 - Incorporación Fiscal',
    '625': '625 - Actividades Agrícolas, Ganaderas, Silvícolas y Pesqueras',
    '626': '626 - Régimen Simplificado de Confianza (RESICO)'
}

USOS = {
    'G01': 'G01 - Adquisición de mercancías',
    'G02': 'G02 - Devoluciones, descuentos o bonificaciones',
    'G03': 'G03 - Gastos en general',
    'I01': 'I01 - Construcciones',
    'I02': 'I02 - Mobiliario y equipo de oficina por inversiones',
    'I03': 'I03 - Equipo de transporte por inversiones',
    'I04': 'I04 - Equipo de cómputo y accesorios por inversiones',
    'I08': 'I08 - Otra maquinaria y equipo por inversiones',
    'D01': 'D01 - Honorarios médicos, dentales y gastos hospitalarios',
    'D02': 'D02 - Gastos médicos por incapacidad o discapacidad',
    'D10': 'D10 - Premios por seguros de gastos médicos',
    'CP01': 'CP01 - Pagos',
    'CN01': 'CN01 - Nómina',
    'S01': 'S01 - Sin efectos fiscales'
}

FORMAS_PAGO = {
    '01': '01 - Efectivo',
    '02': '02 - Cheque nominativo',
    '03': '03 - Transferencia electrónica de fondos',
    '04': '04 - Tarjeta de crédito',
    '05': '05 - Monedero electrónico',
    '06': '06 - Dinero electrónico',
    '08': '08 - Vales de despensa',
    '12': '12 - Dación en pago',
    '17': '17 - Compensación',
    '27': '27 - A satisfacción del acreedor',
    '30': '30 - Aplicación de anticipos',
    '99': '99 - Por definir'
}

METODOS_PAGO = {
    'PUE': 'PUE - Pago en una sola exhibición',
    'PPD': 'PPD - Pago en parcialidades o diferido'
}

def format_total_qr(val):
    try:
        s = f"{float(val):.6f}"
        while s.endswith('0'):
            s = s[:-1]
        if s.endswith('.'):
            s = s + '00'
        return s
    except Exception:
        return str(val)

def numero_a_letras(numero):
    unidades = ["", "UN", "DOS", "TRES", "CUATRO", "CINCO", "SEIS", "SIETE", "OCHO", "NUEVE"]
    decenas = ["", "DIEZ", "VEINTE", "TREINTA", "CUARENTA", "CINCUENTA", "SESENTA", "SETENTA", "OCHENTA", "NOVENTA"]
    especiales = {
        11: "ONCE", 12: "DOCE", 13: "TRECE", 14: "CATORCE", 15: "QUINCE",
        16: "DIECISEIS", 17: "DIECISIETE", 18: "DIECIOCHO", 19: "DIECINUEVE",
        21: "VEINTIUNO", 22: "VEINTIDOS", 23: "VEINTITRES", 24: "VEINTICUATRO",
        25: "VEINTICINCO", 26: "VEINTISEIS", 27: "VEINTISIETE", 28: "VEINTIOCHO", 29: "VEINTINUEVE"
    }
    centenas = ["", "CIENTO", "DOSCIENTOS", "TRESCIENTOS", "CUATROCIENTOS", "QUINIENTOS", "SEISCIENTOS", "SETECIENTOS", "OCHOCIENTOS", "NOVECIENTOS"]

    def _convertir_grupo(n):
        if n == 0:
            return ""
        if n == 100:
            return "CIEN"
        resultado = []
        c = n // 100
        d = (n % 100) // 10
        u = n % 10
        
        if c > 0:
            resultado.append(centenas[c])
            
        resto = n % 100
        if resto in especiales:
            resultado.append(especiales[resto])
        else:
            if d > 0:
                resultado.append(decenas[d])
            if u > 0:
                if d > 0:
                    resultado.append("Y")
                resultado.append(unidades[u])
        return " ".join([x for x in resultado if x])

    try:
        val = float(numero)
        enteros = int(val)
        centavos = int(round((val - enteros) * 100))
        
        if enteros == 0:
            texto_enteros = "CERO"
        else:
            partes = []
            # Millones
            millones = enteros // 1000000
            resto = enteros % 1000000
            if millones > 0:
                if millones == 1:
                    partes.append("UN MILLON")
                else:
                    partes.append(f"{_convertir_grupo(millones)} MILLONES")
            
            # Miles
            miles = resto // 1000
            resto = resto % 1000
            if miles > 0:
                if miles == 1:
                    partes.append("UN MIL")
                else:
                    partes.append(f"{_convertir_grupo(miles)} MIL")
                    
            # Unidades
            if resto > 0:
                partes.append(_convertir_grupo(resto))
                
            texto_enteros = " ".join(partes)

        return f"({texto_enteros} PESOS {centavos:02d}/100 M.N.)".upper()
    except Exception:
        return ""

def parse_cfdi(xml_path):
    namespaces = {
        'cfdi': 'http://www.sat.gob.mx/cfd/4',
        'tfd': 'http://www.sat.gob.mx/TimbreFiscalDigital'
    }
    
    tree = ET.parse(xml_path)
    root = tree.getroot()
    
    data = {}
    
    # Comprobante attributes
    data['version'] = root.attrib.get('Version', '4.0')
    data['serie'] = root.attrib.get('Serie', '')
    data['folio'] = root.attrib.get('Folio', '')
    data['fecha'] = root.attrib.get('Fecha', '')
    data['lugar_expedicion'] = root.attrib.get('LugarExpedicion', '')
    data['no_certificado'] = root.attrib.get('NoCertificado', '')
    data['subtotal'] = float(root.attrib.get('SubTotal', '0.00'))
    data['total'] = float(root.attrib.get('Total', '0.00'))
    data['moneda'] = root.attrib.get('Moneda', 'MXN')
    data['forma_pago'] = root.attrib.get('FormaPago', '')
    data['metodo_pago'] = root.attrib.get('MetodoPago', '')
    data['tipo_comprobante'] = root.attrib.get('TipoDeComprobante', 'I')
    
    # Emisor
    emisor_node = root.find('cfdi:Emisor', namespaces)
    if emisor_node is not None:
        data['emisor_rfc'] = emisor_node.attrib.get('Rfc', '')
        data['emisor_nombre'] = emisor_node.attrib.get('Nombre', '')
        data['emisor_regimen'] = emisor_node.attrib.get('RegimenFiscal', '')
    else:
        data['emisor_rfc'] = ''
        data['emisor_nombre'] = ''
        data['emisor_regimen'] = ''
        
    # Receptor
    receptor_node = root.find('cfdi:Receptor', namespaces)
    if receptor_node is not None:
        data['receptor_rfc'] = receptor_node.attrib.get('Rfc', '')
        data['receptor_nombre'] = receptor_node.attrib.get('Nombre', '')
        data['receptor_domicilio'] = receptor_node.attrib.get('DomicilioFiscalReceptor', '')
        data['receptor_regimen'] = receptor_node.attrib.get('RegimenFiscalReceptor', '')
        data['receptor_uso'] = receptor_node.attrib.get('UsoCFDI', '')
    else:
        data['receptor_rfc'] = ''
        data['receptor_nombre'] = ''
        data['receptor_domicilio'] = ''
        data['receptor_regimen'] = ''
        data['receptor_uso'] = ''
        
    # Conceptos
    data['conceptos'] = []
    conceptos_node = root.find('cfdi:Conceptos', namespaces)
    if conceptos_node is not None:
        for c_node in conceptos_node.findall('cfdi:Concepto', namespaces):
            c_data = {
                'clave_prod_serv': c_node.attrib.get('ClaveProdServ', ''),
                'cantidad': c_node.attrib.get('Cantidad', '1'),
                'clave_unidad': c_node.attrib.get('ClaveUnidad', ''),
                'descripcion': c_node.attrib.get('Descripcion', ''),
                'valor_unitario': float(c_node.attrib.get('ValorUnitario', '0.00')),
                'importe': float(c_node.attrib.get('Importe', '0.00')),
                'impuestos': []
            }
            
            # Inner concept taxes (traslados)
            traslados_concept = c_node.findall('.//cfdi:Traslado', namespaces)
            for t_node in traslados_concept:
                t_data = {
                    'impuesto': t_node.attrib.get('Impuesto', '002'),
                    'tipo_factor': t_node.attrib.get('TipoFactor', 'Tasa'),
                    'tasa_cuota': float(t_node.attrib.get('TasaOCuota', '0.00')),
                    'importe': float(t_node.attrib.get('Importe', '0.00')),
                    'base': float(t_node.attrib.get('Base', '0.00'))
                }
                c_data['impuestos'].append(t_data)
            
            data['conceptos'].append(c_data)
            
    # Impuestos Totales
    data['impuestos_trasladados'] = []
    impuestos_node = root.find('cfdi:Impuestos', namespaces)
    if impuestos_node is not None:
        traslados_node = impuestos_node.find('cfdi:Traslados', namespaces)
        if traslados_node is not None:
            for t_node in traslados_node.findall('cfdi:Traslado', namespaces):
                t_data = {
                    'impuesto': t_node.attrib.get('Impuesto', '002'),
                    'tipo_factor': t_node.attrib.get('TipoFactor', 'Tasa'),
                    'tasa_cuota': float(t_node.attrib.get('TasaOCuota', '0.00')),
                    'importe': float(t_node.attrib.get('Importe', '0.00'))
                }
                data['impuestos_trasladados'].append(t_data)

    # Complemento / TimbreFiscalDigital
    tfd_node = root.find('.//tfd:TimbreFiscalDigital', namespaces)
    if tfd_node is not None:
        data['uuid'] = tfd_node.attrib.get('UUID', '')
        data['fecha_timbrado'] = tfd_node.attrib.get('FechaTimbrado', '')
        data['no_certificado_sat'] = tfd_node.attrib.get('NoCertificadoSAT', '')
        data['sello_cfd'] = tfd_node.attrib.get('SelloCFD', '')
        data['sello_sat'] = tfd_node.attrib.get('SelloSAT', '')
        data['rfc_prov_certif'] = tfd_node.attrib.get('RfcProvCertif', '')
    else:
        data['uuid'] = ''
        data['fecha_timbrado'] = ''
        data['no_certificado_sat'] = ''
        data['sello_cfd'] = ''
        data['sello_sat'] = ''
        data['rfc_prov_certif'] = ''
        
    return data

def build_pdf(data, output_path):
    # Setup document: letter size, 0.5 in margins
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=36,
        rightMargin=36,
        topMargin=36,
        bottomMargin=36
    )
    
    styles = getSampleStyleSheet()
    
    # Custom styles matching visual guidelines
    # We will use slate/navy theme (#0f172a / #1e293b)
    color_primary = colors.HexColor('#0f172a')
    color_secondary = colors.HexColor('#1e293b')
    color_border = colors.HexColor('#cbd5e1')
    color_bg_header = colors.HexColor('#f1f5f9')
    color_bg_alternating = colors.HexColor('#f8fafc')
    
    style_normal = ParagraphStyle(
        'CFDINormal',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=10,
        textColor=colors.HexColor('#334155')
    )
    
    style_bold = ParagraphStyle(
        'CFDIBold',
        parent=style_normal,
        fontName='Helvetica-Bold'
    )
    
    style_title = ParagraphStyle(
        'CFDITitle',
        parent=style_normal,
        fontName='Helvetica-Bold',
        fontSize=14,
        leading=16,
        textColor=color_primary
    )
    
    style_table_header = ParagraphStyle(
        'CFDITableHeader',
        parent=style_normal,
        fontName='Helvetica-Bold',
        fontSize=8,
        leading=9,
        textColor=colors.white
    )
    
    style_small = ParagraphStyle(
        'CFDISmall',
        parent=style_normal,
        fontSize=6,
        leading=8,
        textColor=colors.HexColor('#475569')
    )
    
    style_small_bold = ParagraphStyle(
        'CFDISmallBold',
        parent=style_small,
        fontName='Helvetica-Bold'
    )

    story = []
    
    # ----------------------------------------------------
    # Header Section (Emisor on Left, Factura Details on Right)
    # ----------------------------------------------------
    emisor_desc = f"<b>{data['emisor_nombre']}</b><br/>" \
                  f"RFC: {data['emisor_rfc']}<br/>" \
                  f"Régimen Fiscal: {REGIMENES.get(data['emisor_regimen'], data['emisor_regimen'])}"
                  
    emisor_para = Paragraph(emisor_desc, style_normal)
    
    factura_desc = f"<font size=14><b>FACTURA</b></font><br/>" \
                   f"<b>Folio:</b> {data['serie']} {data['folio']}<br/>" \
                   f"<b>Folio Fiscal (UUID):</b> {data['uuid']}<br/>" \
                   f"<b>Lugar de Expedición:</b> {data['lugar_expedicion']}<br/>" \
                   f"<b>Fecha de Emisión:</b> {data['fecha']}"
                   
    factura_para = Paragraph(factura_desc, style_normal)
    
    header_table = Table([[emisor_para, factura_para]], colWidths=[310, 230])
    header_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('PADDING', (0,0), (-1,-1), 0),
        ('BOTTOMPADDING', (0,0), (-1,-1), 10),
    ]))
    story.append(header_table)
    
    # ----------------------------------------------------
    # Receptor Section (Datos Receptor)
    # ----------------------------------------------------
    receptor_header = Table([[Paragraph("<b>DATOS DEL RECEPTOR</b>", ParagraphStyle('RecHead', parent=style_bold, textColor=color_primary))]], colWidths=[540])
    receptor_header.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), color_bg_header),
        ('PADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('TOPPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(receptor_header)
    story.append(Spacer(1, 4))
    
    rec_col1 = f"<b>RFC:</b> {data['receptor_rfc']}<br/>" \
               f"<b>Nombre:</b> {data['receptor_nombre']}<br/>" \
               f"<b>Régimen Fiscal:</b> {REGIMENES.get(data['receptor_regimen'], data['receptor_regimen'])}"
               
    rec_col2 = f"<b>Domicilio Fiscal (C.P.):</b> {data['receptor_domicilio']}<br/>" \
               f"<b>Uso CFDI:</b> {USOS.get(data['receptor_uso'], data['receptor_uso'])}"
               
    receptor_body = Table([[Paragraph(rec_col1, style_normal), Paragraph(rec_col2, style_normal)]], colWidths=[270, 270])
    receptor_body.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('PADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 10),
    ]))
    story.append(receptor_body)
    
    # ----------------------------------------------------
    # Concepts Table Section
    # ----------------------------------------------------
    concepts_header = [
        Paragraph("Cve Prod/Serv", style_table_header),
        Paragraph("Cant.", style_table_header),
        Paragraph("Unidad", style_table_header),
        Paragraph("Descripción", style_table_header),
        Paragraph("P. Unitario", style_table_header),
        Paragraph("Importe", style_table_header)
    ]
    
    concepts_data = [concepts_header]
    table_styles = [
        ('BACKGROUND', (0,0), (-1,0), color_primary),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('GRID', (0,0), (-1,-1), 0.5, color_border),
        ('PADDING', (0,0), (-1,-1), 4),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]
    
    row_idx = 1
    for item in data['conceptos']:
        desc_text = f"{item['descripcion']}"
        # If there are taxes associated with the concept, add a sub-text detail line
        if item['impuestos']:
            taxes_str = []
            for tax in item['impuestos']:
                t_type = "IVA" if tax['impuesto'] == '002' else tax['impuesto']
                t_factor = tax['tipo_factor']
                t_tasa = f"{tax['tasa_cuota']*100:.2f}%" if t_factor == 'Tasa' else str(tax['tasa_cuota'])
                taxes_str.append(f"{t_type} {t_factor} {t_tasa} Base: ${tax['base']:.2f} Imp: ${tax['importe']:.2f}")
            desc_text += f"<br/><font color='#64748b' size=7><i>Impuestos Trasladados: {', '.join(taxes_str)}</i></font>"
            
        row = [
            Paragraph(item['clave_prod_serv'], style_normal),
            Paragraph(str(item['cantidad']), ParagraphStyle('RightP', parent=style_normal, alignment=2)),
            Paragraph(item['clave_unidad'], style_normal),
            Paragraph(desc_text, style_normal),
            Paragraph(f"${item['valor_unitario']:.2f}", ParagraphStyle('RightP', parent=style_normal, alignment=2)),
            Paragraph(f"${item['importe']:.2f}", ParagraphStyle('RightP', parent=style_normal, alignment=2))
        ]
        concepts_data.append(row)
        if row_idx % 2 == 0:
            table_styles.append(('BACKGROUND', (0, row_idx), (-1, row_idx), color_bg_alternating))
        row_idx += 1
        
    concepts_table = Table(concepts_data, colWidths=[65, 35, 45, 250, 70, 75])
    concepts_table.setStyle(TableStyle(table_styles))
    story.append(concepts_table)
    story.append(Spacer(1, 10))
    
    # ----------------------------------------------------
    # Totals Block Section (Totals + Total with Letra)
    # ----------------------------------------------------
    total_letras_desc = f"<b>Total con letra:</b><br/>{numero_a_letras(data['total'])}"
    total_letras_para = Paragraph(total_letras_desc, style_normal)
    
    totals_rows = [
        [Paragraph("<b>Subtotal:</b>", style_normal), Paragraph(f"${data['subtotal']:.2f}", ParagraphStyle('RightP', parent=style_normal, alignment=2))]
    ]
    
    for tx in data['impuestos_trasladados']:
        t_type = "IVA" if tx['impuesto'] == '002' else tx['impuesto']
        t_tasa = f"{tx['tasa_cuota']*100:.2f}%" if tx['tipo_factor'] == 'Tasa' else str(tx['tasa_cuota'])
        totals_rows.append([
            Paragraph(f"<b>{t_type} {t_tasa}:</b>", style_normal),
            Paragraph(f"${tx['importe']:.2f}", ParagraphStyle('RightP', parent=style_normal, alignment=2))
        ])
        
    totals_rows.append([
        Paragraph("<b>Total:</b>", style_bold),
        Paragraph(f"<b>${data['total']:.2f}</b>", ParagraphStyle('RightP', parent=style_bold, alignment=2))
    ])
    
    totals_table = Table(totals_rows, colWidths=[100, 100])
    totals_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('PADDING', (0,0), (-1,-1), 4),
        ('LINEBELOW', (0,0), (-1,-1), 0.5, color_bg_header),
    ]))
    
    totals_block_table = Table([[total_letras_para, totals_table]], colWidths=[340, 200])
    totals_block_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('PADDING', (0,0), (-1,-1), 0),
    ]))
    story.append(totals_block_table)
    story.append(Spacer(1, 15))
    
    # ----------------------------------------------------
    # Stamp Details & QR Code Block (Certificado y SAT)
    # ----------------------------------------------------
    # QR Code standard URL:
    qr_url = f"https://verificacfdi.facturaelectronica.sat.gob.mx/default.aspx" \
             f"?id={data['uuid']}" \
             f"&re={data['emisor_rfc']}" \
             f"&rr={data['receptor_rfc']}" \
             f"&tt={format_total_qr(data['total'])}" \
             f"&fe={data['sello_cfd'][-8:]}"
             
    qr_widget = QrCodeWidget(qr_url)
    qr_widget.barWidth = 80
    qr_widget.barHeight = 80
    qr_drawing = Drawing(80, 80)
    qr_drawing.add(qr_widget)
    
    stamp_desc = f"<b>RFC Proveedor Certificación:</b> {data['rfc_prov_certif']} | " \
                 f"<b>Fecha y Hora de Certificación:</b> {data['fecha_timbrado']}<br/>" \
                 f"<b>No. de Serie del Certificado SAT:</b> {data['no_certificado_sat']} | " \
                 f"<b>No. de Serie del Certificado CSD:</b> {data['no_certificado']}<br/>" \
                 f"<b>Método de Pago:</b> {METODOS_PAGO.get(data['metodo_pago'], data['metodo_pago'])} | " \
                 f"<b>Forma de Pago:</b> {FORMAS_PAGO.get(data['forma_pago'], data['forma_pago'])}<br/>" \
                 f"<b>Moneda:</b> {data['moneda']}"
                 
    stamp_para = Paragraph(stamp_desc, style_small)
    
    sello_cfd_para = Paragraph(f"<b>Sello digital del CFDI:</b><br/>{data['sello_cfd']}", style_small)
    sello_sat_para = Paragraph(f"<b>Sello del SAT:</b><br/>{data['sello_sat']}", style_small)
    
    cadena_orig = f"||1.1|{data['uuid']}|{data['fecha_timbrado']}|{data['rfc_prov_certif']}|{data['sello_cfd']}|{data['no_certificado_sat']}||"
    cadena_para = Paragraph(f"<b>Cadena original del complemento de certificación digital del SAT:</b><br/>{cadena_orig}", style_small)
    
    details_block = [
        stamp_para,
        Spacer(1, 4),
        sello_cfd_para,
        Spacer(1, 4),
        sello_sat_para,
        Spacer(1, 4),
        cadena_para
    ]
    
    bottom_table = Table([[qr_drawing, details_block]], colWidths=[95, 445])
    bottom_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('PADDING', (0,0), (-1,-1), 0),
    ]))
    
    # Wrap in KeepTogether to ensure it doesn't break across pages
    story.append(KeepTogether([
        bottom_table,
        Spacer(1, 15),
        Paragraph("<center><b>ESTE DOCUMENTO ES UNA REPRESENTACION IMPRESA DE UN CFDI. V 4.0</b></center>", style_normal)
    ]))
    
    # Build Document
    doc.build(story)

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 cfdi_to_pdf.py <xml_input_path> <pdf_output_path>")
        sys.exit(1)
        
    xml_path = sys.argv[1]
    pdf_path = sys.argv[2]
    
    if not os.path.exists(xml_path):
        print(f"Error: XML file '{xml_path}' not found.")
        sys.exit(1)
        
    try:
        data = parse_cfdi(xml_path)
        build_pdf(data, pdf_path)
        print("PDF generated successfully.")
    except Exception as e:
        print(f"Error generating PDF: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
