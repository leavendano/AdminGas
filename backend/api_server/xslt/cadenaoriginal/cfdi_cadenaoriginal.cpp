#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

#include <iostream>
#include <string>

bool aplicarXsltACfdi(const std::string& archivoCfdi, const std::string& archivoXslt) {
    // Inicializar libxslt
    exsltRegisterAll();
    
    // Cargar el XSLT
    xsltStylesheetPtr estilo = xsltParseStylesheetFile((const xmlChar*)archivoXslt.c_str());
    if (!estilo) {
        std::cerr << "Error: No se pudo cargar el XSLT" << std::endl;
        return false;
    }
    
    // Cargar el XML del CFDI
    xmlDocPtr cfdi = xmlParseFile(archivoCfdi.c_str());
    if (!cfdi) {
        std::cerr << "Error: No se pudo cargar el CFDI XML" << std::endl;
        xsltFreeStylesheet(estilo);
        return false;
    }
    
    // Aplicar la transformación
    xmlDocPtr resultado = xsltApplyStylesheet(estilo, cfdi, NULL);
    if (!resultado) {
        std::cerr << "Error: No se pudo aplicar la transformación XSLT" << std::endl;
        xmlFreeDoc(cfdi);
        xsltFreeStylesheet(estilo);
        return false;
    }
    
    // Obtener la cadena original
    xmlChar* cadena;
    int longitud;
    xsltSaveResultToString(&cadena, &longitud, resultado, estilo);
    
    if (cadena) {
        std::string cadenaOriginal((char*)cadena, longitud);
        // Limpiar espacios en blanco
        cadenaOriginal.erase(0, cadenaOriginal.find_first_not_of(" \t\n\r"));
        cadenaOriginal.erase(cadenaOriginal.find_last_not_of(" \t\n\r") + 1);
        
        std::cout << "Cadena Original:" << std::endl;
        std::cout << cadenaOriginal << std::endl;
        
        xmlFree(cadena);
    }
    
    // Liberar memoria
    xmlFreeDoc(resultado);
    xmlFreeDoc(cfdi);
    xsltFreeStylesheet(estilo);
    
    return true;
}

int main() {
    std::string archivoCfdi = "cfdi.xml";
    std::string archivoXslt = "cadenaoriginal_4_0.xslt";
    
    if (aplicarXsltACfdi(archivoCfdi, archivoXslt)) {
        std::cout << "Transformación completada exitosamente" << std::endl;
    } else {
        std::cerr << "Error en la transformación" << std::endl;
        return 1;
    }
    
    return 0;
}