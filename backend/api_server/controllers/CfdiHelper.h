#pragma once

#include <string>
#include <functional>
#include "models/Factura.h"
#include "models/Emisor.h"
#include "models/Receptor.h"

namespace admingas {

struct CfdiResult {
    bool success;
    std::string xml;
    std::string uuid;
    std::string sello;
    std::string error;
};

class CfdiHelper {
public:
    // 1. Mapea la Factura a cfdi::Comprobante y genera el XML provisional (sin sello)
    static std::string generarXmlProvisional(
        const drogon_model::admingas::Factura& factura,
        const drogon_model::admingas::Emisor& emisor,
        const drogon_model::admingas::Receptor& receptor,
        std::string& err
    );

    // 2. Aplica la transformación XSLT con cadenaoriginal_4_0.xslt al XML provisional
    static std::string generarCadenaOriginal(
        const std::string& xmlProvisional,
        const std::string& xsltPath,
        std::string& err
    );

    // 3. Genera el sello SHA256+RSA a partir de la cadena original y los datos de la llave privada del emisor
    static std::string generarSello(
        const std::string& cadenaOriginal,
        const std::string& llaveBase64,
        const std::string& llavePassword,
        std::string& err
    );

    // 4. Inserta el sello, noCertificado y certificado base64 en el XML provisional para generar el XML firmado
    static std::string generarXmlFirmado(
        const std::string& xmlProvisional,
        const std::string& selloBase64,
        const std::string& certificadoBase64,
        const std::string& noCertificado,
        std::string& err
    );

    // 5. Envía el XML firmado a Finkok a través de un HTTP POST SOAP y parsea la respuesta
    static void enviarAFinkok(
        const std::string& xmlFirmado,
        std::function<void(CfdiResult)>&& callback
    );
};

} // namespace admingas
