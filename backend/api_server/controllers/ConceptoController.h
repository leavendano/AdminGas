#pragma once

#include <drogon/HttpController.h>
using namespace drogon;

class ConceptoController : public drogon::HttpController<ConceptoController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ConceptoController::get, "/concepto", Get, Options);
    ADD_METHOD_TO(ConceptoController::getOne, "/concepto/{1}", Get, Options);
    ADD_METHOD_TO(ConceptoController::create, "/concepto", Post, Options);
    ADD_METHOD_TO(ConceptoController::updateOne, "/concepto/{1}", Put, Options);
    ADD_METHOD_TO(ConceptoController::deleteOne, "/concepto/{1}", Delete, Options);
    METHOD_LIST_END

    void get(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback);
    void getOne(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string &&id);
    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
    void updateOne(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string &&id);
    void deleteOne(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string &&id);
};
