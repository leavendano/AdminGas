#include "ReceptorController.h"
#include "models/Receptor.h"
#include <string>

using namespace drogon_model::admingas;

void RepublicError(std::function<void(const HttpResponsePtr &)> &callback, const std::string &errorMsg, HttpStatusCode code = k500InternalServerError) {
    Json::Value error;
    error["error"] = errorMsg;
    auto resp = HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(code);
    callback(resp);
}

void ReceptorController::get(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Receptor> mp(dbClient);
    
    try {
        auto receptors = mp.findAll();
        Json::Value ret(Json::arrayValue);
        for (auto const &rec : receptors) {
            ret.append(rec.toJson());
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicError(callback, e.base().what());
    }
}

void ReceptorController::getOne(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback,
                                std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Receptor> mp(dbClient);
    try {
        auto rec = mp.findByPrimaryKey(std::stoi(id));
        auto resp = HttpResponse::newHttpJsonResponse(rec.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicError(callback, e.base().what(), k404NotFound);
    } catch (const std::exception &e) {
        RepublicError(callback, e.what(), k400BadRequest);
    }
}

void ReceptorController::create(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }
    
    try {
        Receptor rec(*jsonPtr);
        auto dbClient = app().getDbClient();
        drogon::orm::Mapper<Receptor> mp(dbClient);
        mp.insert(rec);
        auto resp = HttpResponse::newHttpJsonResponse(rec.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicError(callback, e.what(), k400BadRequest);
    }
}

void ReceptorController::updateOne(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   std::string &&id)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }

    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Receptor> mp(dbClient);
    try {
        auto rec = mp.findByPrimaryKey(std::stoi(id));
        rec.updateByJson(*jsonPtr);
        mp.update(rec);
        auto resp = HttpResponse::newHttpJsonResponse(rec.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicError(callback, e.what(), k400BadRequest);
    }
}

void ReceptorController::deleteOne(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Receptor> mp(dbClient);
    try {
        mp.deleteByPrimaryKey(std::stoi(id));
        Json::Value ret;
        ret["success"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicError(callback, e.what(), k400BadRequest);
    }
}
