#include "EmisorController.h"
#include "models/Emisor.h"
#include <string>
#include <algorithm>
#include <ctime>
#include <drogon/utils/Utilities.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include <openssl/bn.h>

using namespace drogon_model::admingas;

static void RepublicEmisorError(std::function<void(const HttpResponsePtr &)> &callback, const std::string &errorMsg, HttpStatusCode code = k500InternalServerError) {
    Json::Value error;
    error["error"] = errorMsg;
    auto resp = HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(code);
    callback(resp);
}

void EmisorController::get(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Emisor> mp(dbClient);
    
    try {
        auto emisores = mp.findAll();
        Json::Value ret(Json::arrayValue);
        for (auto const &emi : emisores) {
            ret.append(emi.toJson());
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicEmisorError(callback, e.base().what());
    }
}

void EmisorController::getOne(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback,
                              std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Emisor> mp(dbClient);
    try {
        auto emi = mp.findByPrimaryKey(std::stoi(id));
        auto resp = HttpResponse::newHttpJsonResponse(emi.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicEmisorError(callback, e.base().what(), k404NotFound);
    } catch (const std::exception &e) {
        RepublicEmisorError(callback, e.what(), k400BadRequest);
    }
}

void EmisorController::create(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicEmisorError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }
    
    try {
        Emisor emi(*jsonPtr);

        if (jsonPtr->isMember("certificado") && !(*jsonPtr)["certificado"].isNull()) {
            std::string b64 = (*jsonPtr)["certificado"].asString();
            if (!b64.empty()) {
                b64.erase(std::remove_if(b64.begin(), b64.end(), [](unsigned char c) { return std::isspace(c); }), b64.end());
                std::string der = drogon::utils::base64Decode(b64);
                const unsigned char* p = reinterpret_cast<const unsigned char*>(der.data());
                X509* x509 = d2i_X509(nullptr, &p, der.size());
                if (x509) {
                    // Extract Serial Number
                    ASN1_INTEGER* serial = X509_get_serialNumber(x509);
                    BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
                    if (bn) {
                        char* hex = BN_bn2hex(bn);
                        std::string hexStr(hex);
                        OPENSSL_free(hex);
                        BN_free(bn);

                        std::string serialStr = "";
                        for (size_t i = 0; i < hexStr.length(); i += 2) {
                            std::string part = hexStr.substr(i, 2);
                            char ch = (char)std::stoul(part, nullptr, 16);
                            serialStr += ch;
                        }
                        emi.setCertificadoNumero(serialStr);
                    }

                    // Extract Validity Dates
                    const ASN1_TIME* notBefore = X509_get0_notBefore(x509);
                    struct tm tmBefore;
                    if (ASN1_TIME_to_tm(notBefore, &tmBefore)) {
                        tmBefore.tm_isdst = -1;
                        std::time_t epochBefore = std::mktime(&tmBefore);
                        if (epochBefore != -1) {
                            emi.setCertificadoValidoDesde(trantor::Date(epochBefore * 1000000));
                        }
                    }

                    const ASN1_TIME* notAfter = X509_get0_notAfter(x509);
                    struct tm tmAfter;
                    if (ASN1_TIME_to_tm(notAfter, &tmAfter)) {
                        tmAfter.tm_isdst = -1;
                        std::time_t epochAfter = std::mktime(&tmAfter);
                        if (epochAfter != -1) {
                            emi.setCertificadoValidoHasta(trantor::Date(epochAfter * 1000000));
                        }
                    }

                    X509_free(x509);
                } else {
                    RepublicEmisorError(callback, "Failed to parse X509 DER certificate", k400BadRequest);
                    return;
                }
            }
        }

        auto dbClient = app().getDbClient();
        drogon::orm::Mapper<Emisor> mp(dbClient);
        mp.insert(emi);
        auto resp = HttpResponse::newHttpJsonResponse(emi.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicEmisorError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicEmisorError(callback, e.what(), k400BadRequest);
    }
}

void EmisorController::updateOne(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback,
                                 std::string &&id)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        RepublicEmisorError(callback, "Invalid JSON input", k400BadRequest);
        return;
    }

    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Emisor> mp(dbClient);
    try {
        auto emi = mp.findByPrimaryKey(std::stoi(id));

        if (jsonPtr->isMember("certificado") && !(*jsonPtr)["certificado"].isNull()) {
            std::string b64 = (*jsonPtr)["certificado"].asString();
            if (!b64.empty()) {
                b64.erase(std::remove_if(b64.begin(), b64.end(), [](unsigned char c) { return std::isspace(c); }), b64.end());
                std::string der = drogon::utils::base64Decode(b64);
                const unsigned char* p = reinterpret_cast<const unsigned char*>(der.data());
                X509* x509 = d2i_X509(nullptr, &p, der.size());
                if (x509) {
                    // Extract Serial Number
                    ASN1_INTEGER* serial = X509_get_serialNumber(x509);
                    BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
                    if (bn) {
                        char* hex = BN_bn2hex(bn);
                        std::string hexStr(hex);
                        OPENSSL_free(hex);
                        BN_free(bn);

                        std::string serialStr = "";
                        for (size_t i = 0; i < hexStr.length(); i += 2) {
                            std::string part = hexStr.substr(i, 2);
                            char ch = (char)std::stoul(part, nullptr, 16);
                            serialStr += ch;
                        }
                        emi.setCertificadoNumero(serialStr);
                    }

                    // Extract Validity Dates
                    const ASN1_TIME* notBefore = X509_get0_notBefore(x509);
                    struct tm tmBefore;
                    if (ASN1_TIME_to_tm(notBefore, &tmBefore)) {
                        tmBefore.tm_isdst = -1;
                        std::time_t epochBefore = std::mktime(&tmBefore);
                        if (epochBefore != -1) {
                            emi.setCertificadoValidoDesde(trantor::Date(epochBefore * 1000000));
                        }
                    }

                    const ASN1_TIME* notAfter = X509_get0_notAfter(x509);
                    struct tm tmAfter;
                    if (ASN1_TIME_to_tm(notAfter, &tmAfter)) {
                        tmAfter.tm_isdst = -1;
                        std::time_t epochAfter = std::mktime(&tmAfter);
                        if (epochAfter != -1) {
                            emi.setCertificadoValidoHasta(trantor::Date(epochAfter * 1000000));
                        }
                    }

                    X509_free(x509);
                } else {
                    RepublicEmisorError(callback, "Failed to parse X509 DER certificate", k400BadRequest);
                    return;
                }
            }
        }

        emi.updateByJson(*jsonPtr);
        mp.update(emi);
        auto resp = HttpResponse::newHttpJsonResponse(emi.toJson());
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicEmisorError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicEmisorError(callback, e.what(), k400BadRequest);
    }
}

void EmisorController::deleteOne(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback,
                                 std::string &&id)
{
    auto dbClient = app().getDbClient();
    drogon::orm::Mapper<Emisor> mp(dbClient);
    try {
        mp.deleteByPrimaryKey(std::stoi(id));
        Json::Value ret;
        ret["success"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        RepublicEmisorError(callback, e.base().what());
    } catch (const std::exception &e) {
        RepublicEmisorError(callback, e.what(), k400BadRequest);
    }
}
