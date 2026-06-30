#include <drogon/drogon.h>
#include <xercesc/util/PlatformUtils.hpp>

int main() {
    // Inicializar Xerces-C++ para CodeSynthesis XSD
    xercesc::XMLPlatformUtils::Initialize();

    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "http://localhost:5173");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
    });
    
    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr &req, drogon::FilterCallback &&defer, drogon::FilterChainCallback &&chain) {
        if (req->method() == drogon::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "http://localhost:5173");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
            defer(resp);
            return;
        }
        chain();
    });

    drogon::app().loadConfigFile("config.json");
    drogon::app().run();

    // Finalizar Xerces-C++
    xercesc::XMLPlatformUtils::Terminate();
    return 0;
}
