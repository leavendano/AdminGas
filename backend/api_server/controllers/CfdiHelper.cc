#include "CfdiHelper.h"
#include "cfdv40.h"
#include <drogon/utils/Utilities.h>
#include <drogon/HttpClient.h>
#include <json/json.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <memory>

namespace admingas {

// Helper recursivo para buscar nodos por su nombre local (ignorando prefijos de namespace)
static xmlNodePtr buscarNodoPorNombreLocal(xmlNodePtr root, const std::string& nombreLocal) {
    if (!root) return nullptr;
    if (root->name) {
        std::string name((char*)root->name);
        if (name == nombreLocal) return root;
    }
    for (xmlNodePtr child = root->children; child != nullptr; child = child->next) {
        xmlNodePtr res = buscarNodoPorNombreLocal(child, nombreLocal);
        if (res) return res;
    }
    return nullptr;
}

// Helper para extraer el texto de un nodo
static std::string obtenerTextoNodo(xmlNodePtr node) {
    if (!node || !node->children) return "";
    xmlChar* content = xmlNodeGetContent(node);
    std::string text = content ? (char*)content : "";
    if (content) xmlFree(content);
    return text;
}

std::string CfdiHelper::generarXmlProvisional(
    const drogon_model::admingas::Factura& factura,
    const drogon_model::admingas::Emisor& emisor,
    const drogon_model::admingas::Receptor& receptor,
    std::string& err
) {
    try {
        // Formatear fecha actual a formato SAT usando xml_schema::date_time
        std::time_t t = std::time(nullptr);
        std::tm* local = std::localtime(&t);
        xml_schema::date_time dt(
            local->tm_year + 1900,
            local->tm_mon + 1,
            local->tm_mday,
            local->tm_hour,
            local->tm_min,
            local->tm_sec
        );

        // Instanciar Emisor del CFDI
        cfdi::Emisor emi(
            emisor.getValueOfRfc(),
            emisor.getValueOfNombre(),
            catalogos::c_RegimenFiscal(emisor.getValueOfRegimenFiscal())
        );

        // Instanciar Receptor del CFDI
        cfdi::Receptor rec(
            receptor.getValueOfRfc(),
            receptor.getValueOfNombre(),
            tipodatos::t_RFC(receptor.getValueOfDomicilioFiscalReceptor()),
            catalogos::c_RegimenFiscal(receptor.getValueOfRegimenFiscalReceptor()),
            catalogos::c_UsoCFDI(receptor.getValueOfUsoCfdi())
        );

        // Parsear los conceptos guardados como JSON en la base de datos
        Json::Value conceptsJson;
        Json::CharReaderBuilder readerBuilder;
        std::string readerErr;
        std::unique_ptr<Json::CharReader> const jsonReader(readerBuilder.newCharReader());
        bool parsed = jsonReader->parse(
            factura.getValueOfConceptos().c_str(),
            factura.getValueOfConceptos().c_str() + factura.getValueOfConceptos().size(),
            &conceptsJson,
            &readerErr
        );

        if (!parsed || !conceptsJson.isArray()) {
            err = "Error al parsear los conceptos de la factura: " + readerErr;
            return "";
        }

        cfdi::Conceptos concepts;
        double totalImpuestosTrasladados = 0.0;

        for (unsigned int i = 0; i < conceptsJson.size(); ++i) {
            const auto& c = conceptsJson[i];
            
            double cantidadVal = c["cantidad"].asDouble();
            double valorUnitarioVal = c["valor_unitario"].asDouble();
            double subtotalVal = c["subtotal"].asDouble();
            double impuestoVal = c["impuesto"].asDouble();
            
            cfdi::Concepto concepto(
                catalogos::c_ClaveProdServ(c["clave_prod_serv"].asString()),
                cfdi::Cantidad(cantidadVal),
                catalogos::c_ClaveUnidad(c["clave_unidad"].asString()),
                cfdi::Descripcion(c["descripcion"].asString()),
                tipodatos::t_Importe(valorUnitarioVal),
                tipodatos::t_Importe(subtotalVal),
                catalogos::c_ObjetoImp("02") // 02 = Sí objeto de impuesto
            );

            // Agregar impuesto trasladado al concepto (IVA)
            cfdi::Traslado1 traslado(
                cfdi::Base(subtotalVal),
                catalogos::c_Impuesto("002"), // 002 = IVA
                catalogos::c_TipoFactor("Tasa")
            );

            double ivaPorcentaje = c.isMember("iva_porcentaje") ? c["iva_porcentaje"].asDouble() : 16.0;
            double tasaCuota = ivaPorcentaje / 100.0;
            
            traslado.TasaOCuota(cfdi::TasaOCuota(tasaCuota));
            traslado.Importe(tipodatos::t_Importe(impuestoVal));

            cfdi::Traslados1 traslados;
            traslados.Traslado().push_back(traslado);

            cfdi::Impuestos1 impuestos;
            impuestos.Traslados(traslados);
            
            concepto.Impuestos(impuestos);
            concepts.Concepto().push_back(concepto);

            totalImpuestosTrasladados += impuestoVal;
        }

        // Instanciar el Comprobante (raíz)
        cfdi::Comprobante comp(
            emi,
            rec,
            concepts,
            tipodatos::t_FechaH(dt),
            cfdi::Sello(""),          // Sello vacío provisionalmente
            cfdi::NoCertificado(emisor.getValueOfCertificadoNumero()),
            cfdi::Certificado(""),    // Vacío provisionalmente
            tipodatos::t_Importe(factura.getValueOfSubtotal()),
            catalogos::c_Moneda("MXN"),
            tipodatos::t_Importe(factura.getValueOfTotal()),
            catalogos::c_TipoDeComprobante("I"), // I = Ingreso
            catalogos::c_Exportacion("01"),      // 01 = No aplica exportación
            catalogos::c_CodigoPostal(!emisor.getValueOfDomicilioFiscal().empty() ? emisor.getValueOfDomicilioFiscal() : "30700")
        );

        comp.FormaPago(catalogos::c_FormaPago("03")); // 03 = Transferencia electrónica (default)
        comp.MetodoPago(catalogos::c_MetodoPago("PUE")); // PUE = Pago en una sola exhibición

        // Agregar Impuestos globales a nivel del Comprobante
        cfdi::Impuestos impuestosGlobales;
        impuestosGlobales.TotalImpuestosTrasladados(tipodatos::t_Importe(totalImpuestosTrasladados));

        // Agrupación de traslados globales por tasa (asumimos IVA 16% o 8%)
        cfdi::Traslados trasladosGlobales;
        
        // Sumar bases e importes para cada tasa de IVA presente en la factura
        double base16 = 0.0, imp16 = 0.0;
        double base8 = 0.0, imp8 = 0.0;

        for (unsigned int i = 0; i < conceptsJson.size(); ++i) {
            const auto& c = conceptsJson[i];
            double ivaPorcentaje = c.isMember("iva_porcentaje") ? c["iva_porcentaje"].asDouble() : 16.0;
            if (ivaPorcentaje == 16.0) {
                base16 += c["subtotal"].asDouble();
                imp16 += c["impuesto"].asDouble();
            } else if (ivaPorcentaje == 8.0) {
                base8 += c["subtotal"].asDouble();
                imp8 += c["impuesto"].asDouble();
            }
        }

        if (base16 > 0.0) {
            cfdi::Traslado trasladoGlobal(
                tipodatos::t_Importe(base16),
                catalogos::c_Impuesto("002"),
                catalogos::c_TipoFactor("Tasa")
            );
            trasladoGlobal.TasaOCuota(cfdi::TasaOCuota(0.160000));
            trasladoGlobal.Importe(tipodatos::t_Importe(imp16));
            trasladosGlobales.Traslado().push_back(trasladoGlobal);
        }

        if (base8 > 0.0) {
            cfdi::Traslado trasladoGlobal(
                tipodatos::t_Importe(base8),
                catalogos::c_Impuesto("002"),
                catalogos::c_TipoFactor("Tasa")
            );
            trasladoGlobal.TasaOCuota(cfdi::TasaOCuota(0.080000));
            trasladoGlobal.Importe(tipodatos::t_Importe(imp8));
            trasladosGlobales.Traslado().push_back(trasladoGlobal);
        }

        impuestosGlobales.Traslados(trasladosGlobales);
        comp.Impuestos(impuestosGlobales);

        // Serializar el Comprobante a un flujo XML string
        std::ostringstream oss;
        xml_schema::namespace_infomap m;
        m["cfdi"].name = "http://www.sat.gob.mx/cfd/4";
        m["cfdi"].schema = "http://www.sat.gob.mx/sitio_internet/cfd/4/cfdv40.xsd";
        m["xsi"].name = "http://www.w3.org/2001/XMLSchema-instance";

        cfdi::Comprobante_(oss, comp, m);
        std::string xmlStr = oss.str();
        
        // Formatear TasaOCuota a 6 decimales para cumplir con el catálogo del SAT
        size_t pos = 0;
        while ((pos = xmlStr.find("TasaOCuota=\"0.16\"", pos)) != std::string::npos) {
            xmlStr.replace(pos, 17, "TasaOCuota=\"0.160000\"");
            pos += 21;
        }
        pos = 0;
        while ((pos = xmlStr.find("TasaOCuota=\"0.08\"", pos)) != std::string::npos) {
            xmlStr.replace(pos, 17, "TasaOCuota=\"0.080000\"");
            pos += 21;
        }

        std::ofstream outXml("provisional.xml");
        outXml << xmlStr;
        outXml.close();
        return xmlStr;

    } catch (const std::exception& e) {
        err = std::string("Error generando el objeto Comprobante: ") + e.what();
        return "";
    }
}

std::string CfdiHelper::generarCadenaOriginal(
    const std::string& xmlProvisional,
    const std::string& xsltPath,
    std::string& err
) {
    // Inicializar libxslt y libexslt
    exsltRegisterAll();

    // Cargar el stylesheet XSLT
    xsltStylesheetPtr sheet = xsltParseStylesheetFile((const xmlChar*)xsltPath.c_str());
    if (!sheet) {
        err = "No se pudo cargar la hoja de estilo XSLT en: " + xsltPath;
        return "";
    }

    // Parsear el XML provisional en memoria
    xmlDocPtr doc = xmlReadMemory(xmlProvisional.c_str(), xmlProvisional.size(), nullptr, "UTF-8", 0);
    if (!doc) {
        err = "No se pudo parsear el XML provisional de la factura.";
        xsltFreeStylesheet(sheet);
        return "";
    }

    // Aplicar la transformación XSLT
    xmlDocPtr res = xsltApplyStylesheet(sheet, doc, nullptr);
    if (!res) {
        err = "Falló la transformación XSLT del CFDI.";
        xmlFreeDoc(doc);
        xsltFreeStylesheet(sheet);
        return "";
    }

    // Obtener el string de salida de la transformación
    xmlChar* cadenaResult = nullptr;
    int longitud = 0;
    xsltSaveResultToString(&cadenaResult, &longitud, res, sheet);

    std::string cadenaOriginal = "";
    if (cadenaResult) {
        cadenaOriginal = std::string((char*)cadenaResult, longitud);
        
        // Eliminar espacios en blanco y saltos de línea al principio y final
        cadenaOriginal.erase(0, cadenaOriginal.find_first_not_of(" \t\n\r"));
        cadenaOriginal.erase(cadenaOriginal.find_last_not_of(" \t\n\r") + 1);

        xmlFree(cadenaResult);
    } else {
        err = "La transformación XSLT no generó ningún resultado.";
    }

    // Limpiar memoria
    xmlFreeDoc(res);
    xmlFreeDoc(doc);
    xsltFreeStylesheet(sheet);

    std::ofstream outCad("cadena.txt");
    outCad << cadenaOriginal;
    outCad.close();

    return cadenaOriginal;
}

std::string CfdiHelper::generarSello(
    const std::string& cadenaOriginal,
    const std::string& llaveBase64,
    const std::string& llavePassword,
    std::string& err
) {
    // Inicializar algoritmos criptográficos
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // Decodificar la clave privada Base64 de la base de datos a formato DER binario
    std::string llaveDer = drogon::utils::base64Decode(llaveBase64);
    if (llaveDer.empty()) {
        err = "Error al decodificar la clave privada base64.";
        return "";
    }

    // Cargar clave privada PKCS8 cifrada en formato DER
    BIO* bio = BIO_new_mem_buf(llaveDer.data(), llaveDer.size());
    EVP_PKEY* pkey = d2i_PKCS8PrivateKey_bio(bio, nullptr, nullptr, (void*)llavePassword.c_str());
    BIO_free(bio);

    if (!pkey) {
        char errBuf[256];
        ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));
        err = std::string("No se pudo cargar la clave privada del emisor (contraseña incorrecta o clave dañada): ") + errBuf;
        return "";
    }

    // Firmar con SHA-256 con RSA
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        err = "Error al crear el contexto EVP_MD_CTX.";
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
        err = "Error al inicializar la firma digest.";
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignUpdate(ctx, cadenaOriginal.data(), cadenaOriginal.size()) <= 0) {
        err = "Error al actualizar la firma digest.";
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) <= 0) {
        err = "Error al obtener la longitud de la firma digest.";
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    std::vector<unsigned char> sig(sigLen);
    if (EVP_DigestSignFinal(ctx, sig.data(), &sigLen) <= 0) {
        err = "Error al generar la firma digest final.";
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    // Codificar la firma en Base64 para el atributo Sello del SAT
    std::string selloBase64 = drogon::utils::base64Encode(sig.data(), sig.size());
    
    // Remover saltos de línea del string Base64 si los tuviera
    selloBase64.erase(std::remove_if(selloBase64.begin(), selloBase64.end(), [](unsigned char c) { return std::isspace(c); }), selloBase64.end());

    return selloBase64;
}

std::string CfdiHelper::generarXmlFirmado(
    const std::string& xmlProvisional,
    const std::string& selloBase64,
    const std::string& certificadoBase64,
    const std::string& noCertificado,
    std::string& err
) {
    // Parsear el XML provisional con libxml2
    xmlDocPtr doc = xmlReadMemory(xmlProvisional.c_str(), xmlProvisional.size(), nullptr, "UTF-8", 0);
    if (!doc) {
        err = "No se pudo cargar el XML provisional para inyectar el sello.";
        return "";
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        err = "El XML provisional carece de nodo raíz.";
        xmlFreeDoc(doc);
        return "";
    }

    // Remover saltos de línea del certificado base64
    std::string certLimpio = certificadoBase64;
    certLimpio.erase(std::remove_if(certLimpio.begin(), certLimpio.end(), [](unsigned char c) { return std::isspace(c); }), certLimpio.end());

    // Inyectar atributos de firma en la raíz (actualizando los existentes generados vacíos por XSD)
    xmlSetProp(root, (const xmlChar*)"Sello", (const xmlChar*)selloBase64.c_str());
    xmlSetProp(root, (const xmlChar*)"NoCertificado", (const xmlChar*)noCertificado.c_str());
    xmlSetProp(root, (const xmlChar*)"Certificado", (const xmlChar*)certLimpio.c_str());

    // Exportar el XML a string
    xmlChar* xmlBuff = nullptr;
    int buffSize = 0;
    xmlDocDumpFormatMemoryEnc(doc, &xmlBuff, &buffSize, "UTF-8", 1);

    std::string xmlFirmado = "";
    if (xmlBuff) {
        xmlFirmado = std::string((char*)xmlBuff, buffSize);
        xmlFree(xmlBuff);
    } else {
        err = "No se pudo volcar el XML con sello a memoria.";
    }

    xmlFreeDoc(doc);
    std::ofstream outFirm("firmado.xml");
    outFirm << xmlFirmado;
    outFirm.close();
    return xmlFirmado;
}

void CfdiHelper::enviarAFinkok(
    const std::string& xmlFirmado,
    std::function<void(CfdiResult)>&& callback
) {
    // 1. Codificar el XML firmado en Base64
    std::string xmlBase64 = drogon::utils::base64Encode((const unsigned char*)xmlFirmado.data(), xmlFirmado.size());
    // Limpiar saltos de línea del Base64
    xmlBase64.erase(std::remove_if(xmlBase64.begin(), xmlBase64.end(), [](unsigned char c) { return std::isspace(c); }), xmlBase64.end());

    // 2. Construir el XML del SOAP Envelope
    std::string soapBody = 
        "<soap-env:Envelope xmlns:soap-env=\"http://schemas.xmlsoap.org/soap/envelope/\">"
        "    <soap-env:Body>"
        "        <ns0:stamp xmlns:ns0=\"http://facturacion.finkok.com/stamp\">"
        "            <ns0:xml>" + xmlBase64 + "</ns0:xml>"
        "            <ns0:username>leavendano@gmail.com</ns0:username>"
        "            <ns0:password>W7y1g5z*</ns0:password>"
        "        </ns0:stamp>"
        "    </soap-env:Body>"
        "</soap-env:Envelope>";

    // 3. Crear el cliente HTTP apuntando a demo-facturacion.finkok.com en puerto 443 (HTTPS)
    auto client = drogon::HttpClient::newHttpClient("https://demo-facturacion.finkok.com");
    
    // 4. Configurar la petición HTTP
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/servicios/soap/stamp");
    req->addHeader("Content-Type", "text/xml; charset=utf-8");
    req->setBody(soapBody);

    // 5. Enviar la petición asíncronamente
    client->sendRequest(req, [callback = std::move(callback)](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
        CfdiResult ret;
        ret.success = false;

        if (res != drogon::ReqResult::Ok || !resp) {
            ret.error = "Error de red al conectar con el PAC Finkok. Intentelo de nuevo.";
            callback(ret);
            return;
        }

        if (resp->getStatusCode() != drogon::k200OK && resp->getStatusCode() != drogon::k500InternalServerError) {
            ret.error = "Finkok respondio con codigo HTTP invalido: " + std::to_string(resp->getStatusCode());
            callback(ret);
            return;
        }

        std::string xmlResponse = std::string(resp->getBody());

        // Parsear la respuesta SOAP
        xmlDocPtr doc = xmlReadMemory(xmlResponse.c_str(), xmlResponse.size(), nullptr, "UTF-8", 0);
        if (!doc) {
            ret.error = "Error al parsear la respuesta SOAP de Finkok.";
            callback(ret);
            return;
        }

        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (!root) {
            ret.error = "Respuesta SOAP vacia de Finkok.";
            xmlFreeDoc(doc);
            callback(ret);
            return;
        }

        // Buscar incidencias de error
        xmlNodePtr incidenciasNode = buscarNodoPorNombreLocal(root, "Incidencias");
        if (incidenciasNode) {
            xmlNodePtr incidenciaNode = buscarNodoPorNombreLocal(incidenciasNode, "Incidencia");
            if (incidenciaNode) {
                xmlNodePtr msgNode = buscarNodoPorNombreLocal(incidenciaNode, "MensajeIncidencia");
                xmlNodePtr codeNode = buscarNodoPorNombreLocal(incidenciaNode, "Codigo");
                
                std::string msg = msgNode ? obtenerTextoNodo(msgNode) : "Error desconocido de Finkok";
                std::string code = codeNode ? obtenerTextoNodo(codeNode) : "";
                
                ret.error = "Rechazo del PAC (" + code + "): " + msg;
                xmlFreeDoc(doc);
                callback(ret);
                return;
            }
        }

        // Extraer XML timbrado y UUID
        xmlNodePtr xmlNode = buscarNodoPorNombreLocal(root, "xml");
        xmlNodePtr uuidNode = buscarNodoPorNombreLocal(root, "UUID");
        xmlNodePtr selloNode = buscarNodoPorNombreLocal(root, "SatSeal");

        if (xmlNode && uuidNode) {
            ret.success = true;
            ret.xml = obtenerTextoNodo(xmlNode);
            ret.uuid = obtenerTextoNodo(uuidNode);
            ret.sello = selloNode ? obtenerTextoNodo(selloNode) : "";
        } else {
            ret.error = "La respuesta de Finkok no contiene el XML timbrado ni el UUID fiscal.";
        }

        xmlFreeDoc(doc);
        callback(ret);
    });
}

} // namespace admingas
