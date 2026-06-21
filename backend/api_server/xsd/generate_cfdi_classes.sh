#!/bin/bash
# generate_cfdi_classes.sh

# Crear directorio de salida
mkdir -p generated

# Generar clases
xsdcxx cxx-tree \
  --output-dir generated \
  --namespace-map http://www.sat.gob.mx/cfd/4=cfdi \
  --namespace-map http://www.sat.gob.mx/sitio_internet/cfd/catalogos=catalogos \
  --namespace-map http://www.sat.gob.mx/sitio_internet/cfd/tipoDatos/tdCFDI=tipodatos \
  --hxx-suffix .h \
  --cxx-suffix .cpp \
  --generate-serialization \
  --generate-ostream \
  --generate-detach \
  --std c++11 \
  cfdv40.xsd

echo "Clases generadas en el directorio 'generated'"