#include "FacturaController.h"
#include "models/Factura.h"
#include "models/Emisor.h"
#include "models/Receptor.h"
#include "CfdiHelper.h"
#include <string>
#include <ctime>
#include <memory>
#include <json/json.h>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <cmath>

using namespace drogon_model::admingas;
using admingas::CfdiHelper;

void FacturaController::get(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Factura> mp(dbClient);
    
    try {
        auto facturas = mp.findAll();
        Json::Value ret(Json::arrayValue);
        for (auto const &f : facturas) {
            Json::Value fJson = f.toJson();
            
            // Parse conceptos string back to Json::Value and replace in output
            std::string err;
            Json::Value conceptsJson;
            Json::CharReaderBuilder reader;
            std::unique_ptr<Json::CharReader> const jsonReader(reader.newCharReader());
            if (jsonReader->parse(f.getValueOfConceptos().c_str(), f.getValueOfConceptos().c_str() + f.getValueOfConceptos().size(), &conceptsJson, &err)) {
                fJson["conceptos"] = conceptsJson;
            } else {
                fJson["conceptos"] = Json::Value(Json::arrayValue);
            }
            ret.append(fJson);
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void FacturaController::create(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        Json::Value error;
        error["error"] = "Invalid JSON input";
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    try {
        Factura factura;
        
        // Emisor details (linked to id if provided, else null, copy all static strings)
        if (jsonPtr->isMember("emisor_id") && !(*jsonPtr)["emisor_id"].isNull()) {
            factura.setEmisorId((*jsonPtr)["emisor_id"].asInt());
        }
        factura.setEmisorRfc((*jsonPtr)["emisor_rfc"].asString());
        factura.setEmisorNombre((*jsonPtr)["emisor_nombre"].asString());
        factura.setEmisorRegimenFiscal((*jsonPtr)["emisor_regimen_fiscal"].asString());
        
        // Receptor details (linked to id if provided, else null, copy all static strings)
        if (jsonPtr->isMember("receptor_id") && !(*jsonPtr)["receptor_id"].isNull()) {
            factura.setReceptorId((*jsonPtr)["receptor_id"].asInt());
        }
        factura.setReceptorRfc((*jsonPtr)["receptor_rfc"].asString());
        factura.setReceptorNombre((*jsonPtr)["receptor_nombre"].asString());
        factura.setReceptorDomicilioFiscal((*jsonPtr)["receptor_domicilio_fiscal"].asString());
        factura.setReceptorRegimenFiscal((*jsonPtr)["receptor_regimen_fiscal"].asString());
        factura.setReceptorUsoCfdi((*jsonPtr)["receptor_uso_cfdi"].asString());
        
        // Date handling
        if (jsonPtr->isMember("fecha") && !(*jsonPtr)["fecha"].isNull()) {
            std::string dateStr = (*jsonPtr)["fecha"].asString();
            struct std::tm tm = {};
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
                tm.tm_year -= 1900;
                tm.tm_mon -= 1;
                tm.tm_hour = 12; // Avoid timezone offset shifting date
                std::time_t t = std::mktime(&tm);
                factura.setFecha(trantor::Date(t * 1000000));
            } else {
                factura.setFecha(trantor::Date::date());
            }
        } else {
            factura.setFecha(trantor::Date::date());
        }
        
        // Payment method (Forma de pago)
        if (jsonPtr->isMember("forma_pago") && !(*jsonPtr)["forma_pago"].isNull()) {
            factura.setFormaPago((*jsonPtr)["forma_pago"].asString());
        } else {
            factura.setFormaPago("03"); // default to 03 (Transferencia)
        }
        
        // Round to 2 decimals
        double subtotal = std::round((*jsonPtr)["subtotal"].asDouble() * 100.0) / 100.0;
        double impuestos = std::round((*jsonPtr)["impuestos"].asDouble() * 100.0) / 100.0;
        double total = std::round((*jsonPtr)["total"].asDouble() * 100.0) / 100.0;

        factura.setSubtotal(subtotal);
        factura.setImpuestos(impuestos);
        factura.setTotal(total);
        
        // Concepts serialization with rounding
        Json::Value conceptosArray(Json::arrayValue);
        if (jsonPtr->isMember("conceptos")) {
            if ((*jsonPtr)["conceptos"].isString()) {
                Json::CharReaderBuilder reader;
                std::string conceptsStr = (*jsonPtr)["conceptos"].asString();
                std::string readerErr;
                std::unique_ptr<Json::CharReader> const jsonReader(reader.newCharReader());
                jsonReader->parse(conceptsStr.c_str(), conceptsStr.c_str() + conceptsStr.size(), &conceptosArray, &readerErr);
            } else if ((*jsonPtr)["conceptos"].isArray()) {
                conceptosArray = (*jsonPtr)["conceptos"];
            }
        }
        
        if (conceptosArray.isArray()) {
            for (auto &concept : conceptosArray) {
                if (concept.isMember("valor_unitario")) {
                    concept["valor_unitario"] = std::round(concept["valor_unitario"].asDouble() * 100.0) / 100.0;
                }
                if (concept.isMember("subtotal")) {
                    concept["subtotal"] = std::round(concept["subtotal"].asDouble() * 100.0) / 100.0;
                }
                if (concept.isMember("impuesto")) {
                    concept["impuesto"] = std::round(concept["impuesto"].asDouble() * 100.0) / 100.0;
                }
                if (concept.isMember("total")) {
                    concept["total"] = std::round(concept["total"].asDouble() * 100.0) / 100.0;
                }
                if (concept.isMember("cantidad")) {
                    concept["cantidad"] = std::round(concept["cantidad"].asDouble() * 100.0) / 100.0;
                }
            }
            Json::StreamWriterBuilder writer;
            std::string conceptsStr = Json::writeString(writer, conceptosArray);
            factura.setConceptos(conceptsStr);
        } else {
            factura.setConceptos("[]");
        }
        
        auto dbClient = app().getDbClient();
        drogon::orm::Mapper<Factura> mp(dbClient);
        mp.insert(factura);
        
        // Return created invoice with concepts as JSON array
        Json::Value fJson = factura.toJson();
        std::string err;
        Json::Value conceptsJson;
        Json::CharReaderBuilder reader;
        std::unique_ptr<Json::CharReader> const jsonReader(reader.newCharReader());
        if (jsonReader->parse(factura.getValueOfConceptos().c_str(), factura.getValueOfConceptos().c_str() + factura.getValueOfConceptos().size(), &conceptsJson, &err)) {
            fJson["conceptos"] = conceptsJson;
        } else {
            fJson["conceptos"] = Json::Value(Json::arrayValue);
        }
        
        auto resp = HttpResponse::newHttpJsonResponse(fJson);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

void FacturaController::deleteOne(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback,
                                  std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Factura> mp(dbClient);
    try {
        mp.deleteByPrimaryKey(std::stoi(id));
        Json::Value ret;
        ret["success"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

void FacturaController::timbrar(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback,
                                std::string &&id)
{
    auto dbClient = app().getDbClient();
    
    try {
        int facturaId = std::stoi(id);
        
        // 1. Obtener la factura de la base de datos
        drogon::orm::Mapper<Factura> mpFactura(dbClient);
        auto factura = mpFactura.findByPrimaryKey(facturaId);
        
        // Validar si la factura ya está timbrada
        if (factura.getValueOfStatus() == "timbrada") {
            Json::Value error;
            error["error"] = "La factura ya se encuentra timbrada.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 2. Obtener Emisor asociado
        if (!factura.getEmisorId()) {
            Json::Value error;
            error["error"] = "La factura no tiene un emisor asignado.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        drogon::orm::Mapper<Emisor> mpEmisor(dbClient);
        auto emisor = mpEmisor.findByPrimaryKey(*(factura.getEmisorId()));

        // Validar que el emisor tenga llave y certificado
        if (emisor.getValueOfCertificado().empty() || emisor.getValueOfLlave().empty()) {
            Json::Value error;
            error["error"] = "El emisor asociado no tiene certificado o clave privada cargados.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 3. Obtener Receptor asociado
        if (!factura.getReceptorId()) {
            Json::Value error;
            error["error"] = "La factura no tiene un receptor asignado.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        drogon::orm::Mapper<Receptor> mpReceptor(dbClient);
        auto receptor = mpReceptor.findByPrimaryKey(*(factura.getReceptorId()));

        // 4. Generar XML provisional
        std::string err;
        std::string xmlProvisional = CfdiHelper::generarXmlProvisional(factura, emisor, receptor, err);
        if (xmlProvisional.empty()) {
            Json::Value error;
            error["error"] = "Error al generar XML provisional: " + err;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 5. Generar Cadena Original
        std::string xsltPath = "./xslt/cadenaoriginal/cadenaoriginal_4_0.xslt";
        std::string cadenaOriginal = CfdiHelper::generarCadenaOriginal(xmlProvisional, xsltPath, err);
        if (cadenaOriginal.empty()) {
            Json::Value error;
            error["error"] = "Error al generar cadena original: " + err;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 6. Generar Sello Digital (SHA256+RSA)
        std::string sello = CfdiHelper::generarSello(
            cadenaOriginal, 
            emisor.getValueOfLlave(), 
            emisor.getValueOfLlavePassword(), 
            err
        );
        if (sello.empty()) {
            Json::Value error;
            error["error"] = "Error al firmar cadena original: " + err;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 7. Generar XML firmado
        std::string xmlFirmado = CfdiHelper::generarXmlFirmado(
            xmlProvisional,
            sello,
            emisor.getValueOfCertificado(),
            emisor.getValueOfCertificadoNumero(),
            err
        );
        if (xmlFirmado.empty()) {
            Json::Value error;
            error["error"] = "Error al insertar sello en XML: " + err;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 8. Enviar al PAC Finkok asíncronamente
        CfdiHelper::enviarAFinkok(xmlFirmado, [
            callback, 
            facturaId, 
            dbClient, 
            sello
        ](admingas::CfdiResult res) {
            if (!res.success) {
                Json::Value error;
                error["error"] = "Error al timbrar con el PAC: " + res.error;
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }

            try {
                // Actualizar la factura en la base de datos
                drogon::orm::Mapper<Factura> mp(dbClient);
                auto fac = mp.findByPrimaryKey(facturaId);
                fac.setXmlTimbrado(res.xml);
                fac.setUuid(res.uuid);
                fac.setSello(sello);
                fac.setStatus("timbrada");
                mp.update(fac);

                // Responder éxito
                Json::Value ret;
                ret["success"] = true;
                ret["uuid"] = res.uuid;
                ret["status"] = "timbrada";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                callback(resp);
            } catch (const std::exception& e) {
                Json::Value error;
                error["error"] = std::string("Timbrado exitoso, pero fallo al guardar en base de datos: ") + e.what();
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            }
        });

    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

void FacturaController::getXml(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback,
                               std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Factura> mp(dbClient);
    
    try {
        auto factura = mp.findByPrimaryKey(std::stoi(id));
        
        if (factura.getValueOfStatus() != "timbrada" || factura.getValueOfXmlTimbrado().empty()) {
            Json::Value error;
            error["error"] = "La factura no se encuentra timbrada o no cuenta con XML.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody(factura.getValueOfXmlTimbrado());
        resp->addHeader("Content-Type", "application/xml; charset=utf-8");
        resp->addHeader("Content-Disposition", "attachment; filename=\"CFDI_" + id + "_" + factura.getValueOfUuid() + ".xml\"");
        callback(resp);
        
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

void FacturaController::getPdf(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback,
                               std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Factura> mp(dbClient);
    
    try {
        auto factura = mp.findByPrimaryKey(std::stoi(id));
        
        if (factura.getValueOfStatus() != "timbrada" || factura.getValueOfXmlTimbrado().empty()) {
            Json::Value error;
            error["error"] = "La factura no se encuentra timbrada o no cuenta con XML.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        
        // Generate unique temporary filenames in the uploads folder
        std::string randSuffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::string xmlTempPath = "./uploads/temp_" + id + "_" + randSuffix + ".xml";
        std::string pdfTempPath = "./uploads/temp_" + id + "_" + randSuffix + ".pdf";
        
        // Write the xml_timbrado contents to the temp file
        std::ofstream xmlFile(xmlTempPath);
        if (!xmlFile.is_open()) {
            Json::Value error;
            error["error"] = "No se pudo crear el archivo XML temporal en el servidor.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        xmlFile << factura.getValueOfXmlTimbrado();
        xmlFile.close();
        
        // Run Python subprocess to render PDF
        std::string cmd = "python3 ./cfdi_to_pdf.py " + xmlTempPath + " " + pdfTempPath;
        int retCode = std::system(cmd.c_str());
        
        // Remove temporary XML file
        std::remove(xmlTempPath.c_str());
        
        if (retCode != 0) {
            std::remove(pdfTempPath.c_str());
            Json::Value error;
            error["error"] = "Error en el motor de generación de PDF.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        
        // Read the generated PDF bytes
        std::ifstream pdfFile(pdfTempPath, std::ios::binary | std::ios::ate);
        if (!pdfFile.is_open()) {
            std::remove(pdfTempPath.c_str());
            Json::Value error;
            error["error"] = "No se pudo leer el archivo PDF temporal generado.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        
        std::streamsize size = pdfFile.tellg();
        pdfFile.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size);
        if (!pdfFile.read(buffer.data(), size)) {
            pdfFile.close();
            std::remove(pdfTempPath.c_str());
            Json::Value error;
            error["error"] = "Error al leer los bytes del PDF temporal.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        pdfFile.close();
        
        // Remove temporary PDF file
        std::remove(pdfTempPath.c_str());
        
        // Return PDF as download stream
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody(std::string(buffer.begin(), buffer.end()));
        resp->addHeader("Content-Type", "application/pdf");
        resp->addHeader("Content-Disposition", "attachment; filename=\"CFDI_" + id + "_" + factura.getValueOfUuid() + ".pdf\"");
        callback(resp);
        
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

void FacturaController::sendEmail(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback,
                                  std::string &&id)
{
    auto dbClient = app().getDbClient();
    
    try {
        int facturaId = std::stoi(id);
        
        // 1. Fetch factura
        drogon::orm::Mapper<Factura> mpFactura(dbClient);
        auto factura = mpFactura.findByPrimaryKey(facturaId);
        
        if (factura.getValueOfStatus() != "timbrada" || factura.getValueOfXmlTimbrado().empty()) {
            Json::Value error;
            error["error"] = "La factura no se encuentra timbrada o no cuenta con XML.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        // 2. Fetch recipient email from receptor (or body override)
        std::string recipientEmail = "";
        auto jsonPtr = req->getJsonObject();
        if (jsonPtr && jsonPtr->isMember("email") && !(*jsonPtr)["email"].isNull()) {
            recipientEmail = (*jsonPtr)["email"].asString();
        }
        
        if (recipientEmail.empty()) {
            if (factura.getReceptorId()) {
                drogon::orm::Mapper<Receptor> mpReceptor(dbClient);
                auto receptor = mpReceptor.findByPrimaryKey(*(factura.getReceptorId()));
                recipientEmail = receptor.getValueOfEmail();
            }
        }
        
        if (recipientEmail.empty()) {
            Json::Value error;
            error["error"] = "El receptor no cuenta con un correo electrónico registrado y no se proporcionó uno alternativo.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        // 3. Write temp XML
        std::string randSuffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::string xmlTempPath = "./uploads/temp_mail_" + id + "_" + randSuffix + ".xml";
        std::string pdfTempPath = "./uploads/temp_mail_" + id + "_" + randSuffix + ".pdf";
        
        std::ofstream xmlFile(xmlTempPath);
        if (!xmlFile.is_open()) {
            Json::Value error;
            error["error"] = "No se pudo crear el archivo XML temporal para el correo.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        xmlFile << factura.getValueOfXmlTimbrado();
        xmlFile.close();
        
        // 4. Generate PDF using cfdi_to_pdf.py
        std::string pdfCmd = "python3 ./cfdi_to_pdf.py " + xmlTempPath + " " + pdfTempPath;
        int pdfRet = std::system(pdfCmd.c_str());
        if (pdfRet != 0) {
            std::remove(xmlTempPath.c_str());
            std::remove(pdfTempPath.c_str());
            Json::Value error;
            error["error"] = "Error al generar la representación impresa PDF de la factura.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        
        // 5. Send email using send_email.py
        std::string mailCmd = "python3 ./send_email.py --email " + recipientEmail + 
                              " --xml " + xmlTempPath + 
                              " --pdf " + pdfTempPath + 
                              " --uuid " + factura.getValueOfUuid();
                              
        int mailRet = std::system(mailCmd.c_str());
        
        // Cleanup temp files
        std::remove(xmlTempPath.c_str());
        std::remove(pdfTempPath.c_str());
        
        if (mailRet != 0) {
            Json::Value error;
            error["error"] = "Error al enviar el correo a través de SMTP. Verifique la dirección o el servidor.";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        
        Json::Value ret;
        ret["success"] = true;
        ret["recipient"] = recipientEmail;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        
    } catch (const drogon::orm::DrogonDbException &e) {
        Json::Value error;
        error["error"] = e.base().what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    } catch (const std::exception &e) {
        Json::Value error;
        error["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}
