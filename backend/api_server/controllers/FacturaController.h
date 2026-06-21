#pragma once

#include <drogon/HttpController.h>
using namespace drogon;

class FacturaController : public drogon::HttpController<FacturaController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(FacturaController::get, "/factura", Get, Options);
    ADD_METHOD_TO(FacturaController::create, "/factura", Post, Options);
    ADD_METHOD_TO(FacturaController::deleteOne, "/factura/{1}", Delete, Options);
    METHOD_LIST_END

    void get(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback);
    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteOne(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string &&id);
};
