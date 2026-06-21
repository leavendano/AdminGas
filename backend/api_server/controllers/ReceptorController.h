#pragma once

#include <drogon/HttpController.h>
using namespace drogon;

class ReceptorController : public drogon::HttpController<ReceptorController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReceptorController::get, "/receptor", Get, Options);
    ADD_METHOD_TO(ReceptorController::getOne, "/receptor/{1}", Get, Options);
    ADD_METHOD_TO(ReceptorController::create, "/receptor", Post, Options);
    ADD_METHOD_TO(ReceptorController::updateOne, "/receptor/{1}", Put, Options);
    ADD_METHOD_TO(ReceptorController::deleteOne, "/receptor/{1}", Delete, Options);
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
