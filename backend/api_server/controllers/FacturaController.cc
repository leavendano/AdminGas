#include "FacturaController.h"
#include "models/Factura.h"
#include "models/Emisor.h"
#include "models/Receptor.h"
#include "CfdiHelper.h"
#include <string>
#include <ctime>
#include <memory>
#include <json/json.h>

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
        
        factura.setSubtotal((*jsonPtr)["subtotal"].asDouble());
        factura.setImpuestos((*jsonPtr)["impuestos"].asDouble());
        factura.setTotal((*jsonPtr)["total"].asDouble());
        
        // Concepts serialization
        if (jsonPtr->isMember("conceptos")) {
            if ((*jsonPtr)["conceptos"].isString()) {
                factura.setConceptos((*jsonPtr)["conceptos"].asString());
            } else {
                Json::StreamWriterBuilder writer;
                std::string conceptsStr = Json::writeString(writer, (*jsonPtr)["conceptos"]);
                factura.setConceptos(conceptsStr);
            }
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
