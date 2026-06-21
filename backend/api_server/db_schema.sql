-- Schema definition for CFDI 4.0 SAT invoicing
CREATE TABLE IF NOT EXISTS receptor (
    id SERIAL PRIMARY KEY,
    rfc VARCHAR(13) NOT NULL UNIQUE,
    nombre VARCHAR(255) NOT NULL,
    domicilio_fiscal_receptor VARCHAR(5) NOT NULL,
    regimen_fiscal_receptor VARCHAR(3) NOT NULL,
    uso_cfdi VARCHAR(3) NOT NULL
);

CREATE TABLE IF NOT EXISTS emisor (
    id SERIAL PRIMARY KEY,
    rfc VARCHAR(13) NOT NULL UNIQUE,
    nombre VARCHAR(255) NOT NULL,
    regimen_fiscal VARCHAR(3) NOT NULL,
    domicilio_fiscal VARCHAR(5),
    certificado TEXT,
    llave TEXT,
    certificado_nombre VARCHAR(255),
    llave_nombre VARCHAR(255),
    llave_password VARCHAR(255),
    certificado_valido_desde DATE,
    certificado_valido_hasta DATE,
    certificado_numero VARCHAR(20)
);

CREATE TABLE IF NOT EXISTS concepto (
    id SERIAL PRIMARY KEY,
    clave_prod_serv VARCHAR(8) NOT NULL,
    clave_unidad VARCHAR(3) NOT NULL,
    descripcion VARCHAR(255) NOT NULL,
    valor_unitario DOUBLE PRECISION NOT NULL
);

CREATE TABLE IF NOT EXISTS factura (
    id SERIAL PRIMARY KEY,
    emisor_id INTEGER REFERENCES emisor(id) ON DELETE SET NULL,
    emisor_rfc VARCHAR(13) NOT NULL,
    emisor_nombre VARCHAR(255) NOT NULL,
    emisor_regimen_fiscal VARCHAR(3) NOT NULL,
    receptor_id INTEGER REFERENCES receptor(id) ON DELETE SET NULL,
    receptor_rfc VARCHAR(13) NOT NULL,
    receptor_nombre VARCHAR(255) NOT NULL,
    receptor_domicilio_fiscal VARCHAR(5) NOT NULL,
    receptor_regimen_fiscal VARCHAR(3) NOT NULL,
    receptor_uso_cfdi VARCHAR(3) NOT NULL,
    fecha DATE NOT NULL,
    subtotal DOUBLE PRECISION NOT NULL,
    impuestos DOUBLE PRECISION NOT NULL,
    total DOUBLE PRECISION NOT NULL,
    conceptos JSONB NOT NULL
);

