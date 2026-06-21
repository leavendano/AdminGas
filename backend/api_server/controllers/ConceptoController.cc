#include "ConceptoController.h"
#include "models/Concepto.h"
#include <string>

using namespace drogon_model::admingas;

static void RepublicConceptoError(std::function<void(const HttpResponsePtr &)> &callback, const std::string &errorMsg, HttpStatusCode code = k500InternalServerError) {
    Json::Value error;
    error["error"] = errorMsg;
    auto resp = HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(code);
    callback(resp);
}

void ConceptoController::get(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Concepto> mp(dbClient);
    
    try {
        auto conceptos = mp.findAll();
        Json::Value ret(Json::arrayValue);
        for (auto const &con : conceptos) {
            ret.append(con.toJson());
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicConceptoError(callback, e.base().what());
    }
}

void ConceptoController::getOne(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback,
                                std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Concepto> mp(dbClient);
    try {
        auto con = mp.findByPrimaryKey(std::stoi(id));
        auto resp = HttpResponse::newHttpJsonResponse(con.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicConceptoError(callback, e.base().what(), k404NotFound);
    } catch (const std::exception &e) {
        RepublicConceptoError(callback, e.what(), k400BadRequest);
    }
}

void ConceptoController::create(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicConceptoError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }
    
    try {
        Concepto con(*jsonPtr);
        auto dbClient = app().getDbClient();
        drogon::orm::Mapper<Concepto> mp(dbClient);
        mp.insert(con);
        auto resp = HttpResponse::newHttpJsonResponse(con.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicConceptoError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicConceptoError(callback, e.what(), k400BadRequest);
    }
}

void ConceptoController::updateOne(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   std::string &&id)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicConceptoError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }

    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Concepto> mp(dbClient);
    try {
        auto con = mp.findByPrimaryKey(std::stoi(id));
        con.updateByJson(*jsonPtr);
        mp.update(con);
        auto resp = HttpResponse::newHttpJsonResponse(con.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicConceptoError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicConceptoError(callback, e.what(), k400BadRequest);
    }
}

void ConceptoController::deleteOne(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Concepto> mp(dbClient);
    try {
        mp.deleteByPrimaryKey(std::stoi(id));
        Json::Value ret;
        ret["success"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicConceptoError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicConceptoError(callback, e.what(), k400BadRequest);
    }
}
