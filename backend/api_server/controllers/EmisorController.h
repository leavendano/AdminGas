#pragma once

#include <drogon/HttpController.h>
using namespace drogon;

class EmisorController : public drogon::HttpController<EmisorController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(EmisorController::get, "/emisor", Get, Options);
    ADD_METHOD_TO(EmisorController::getOne, "/emisor/{1}", Get, Options);
    ADD_METHOD_TO(EmisorController::create, "/emisor", Post, Options);
    ADD_METHOD_TO(EmisorController::updateOne, "/emisor/{1}", Put, Options);
    ADD_METHOD_TO(EmisorController::deleteOne, "/emisor/{1}", Delete, Options);
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
