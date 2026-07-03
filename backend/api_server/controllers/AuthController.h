#pragma once

#include <drogon/HttpController.h>
#include <json/json.h>
#include <string>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthController::login, "/login", Get, Options);
  ADD_METHOD_TO(AuthController::callback, "/callback", Get, Options);
  ADD_METHOD_TO(AuthController::dashboard, "/dashboard", Get, Options);
  ADD_METHOD_TO(AuthController::logout, "/logout", Get, Options);
  ADD_METHOD_TO(AuthController::status, "/auth/status", Get, Options);
  ADD_METHOD_TO(AuthController::signoutCallback, "/signout-callback-oidc", Get, Options);
  METHOD_LIST_END

  void login(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback);
  void callback(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
  void dashboard(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
  void logout(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
  void status(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
  void signoutCallback(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

private:
  // OIDC Configuration constants
  const std::string OPENID_URL = "https://localhost:7001";
  const std::string CLIENT_ID = "facturacion";
  const std::string CLIENT_SECRET = "Cfdi-49725";
  const std::string REDIRECT_URI = "http://localhost:8080/callback";

  // Security and Hashing Helpers
  std::string generateRandomHex(size_t length);
  std::string sha256(const std::string &str);
  std::string base64UrlEncode(const std::string &str);

  // JWT Parsing Helper
  Json::Value parseJwtPayload(const std::string &jwt);
};
