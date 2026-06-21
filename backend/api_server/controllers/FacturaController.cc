#include "FacturaController.h"
#include "models/Factura.h"
#include <string>
#include <ctime>
#include <memory>
#include <json/json.h>

using namespace drogon_model::admingas;

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
