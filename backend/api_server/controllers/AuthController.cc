#include "AuthController.h"
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include <drogon/HttpClient.h>

// Base64 helper definitions
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

static std::string base64UrlDecode(std::string str) {
    for (char &c : str) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (str.size() % 4 != 0) {
        str.push_back('=');
    }
    return base64_decode(str);
}

// AuthController Implementation

void AuthController::login(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Generate secure state and code_verifier (64 hex characters / 32 bytes)
    std::string state = generateRandomHex(64);
    std::string code_verifier = generateRandomHex(64);

    // Save in session
    auto session = req->getSession();
    session->modify([state, code_verifier](drogon::Session::SessionMap &map) {
        map["state"] = state;
        map["code_verifier"] = code_verifier;
    });
    LOG_INFO << "login: Created session " << session->sessionId() << " with state=" << state;

    // Calculate PKCE challenge
    std::string hashed = sha256(code_verifier);
    std::string code_challenge = base64UrlEncode(hashed);

    // Build authorization redirect URL
    std::string authUrl = OPENID_URL + "/connect/authorize?"
        "response_type=code"
        "&client_id=" + CLIENT_ID +
        "&redirect_uri=" + REDIRECT_URI +
        "&state=" + state +
        "&code_challenge=" + code_challenge +
        "&code_challenge_method=S256";

    auto resp = HttpResponse::newRedirectionResponse(authUrl);
    callback(resp);
}

void AuthController::callback(const HttpRequestPtr &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto code = req->getParameter("code");
    auto state = req->getParameter("state");
    auto session = req->getSession();

    LOG_INFO << "callback: Received state=" << state << " from OIDC. Current session ID=" << session->sessionId();
    if (session->find("state")) {
        LOG_INFO << "callback: Found state in session: " << session->get<std::string>("state");
    } else {
        LOG_WARN << "callback: No state found in session!";
    }

    // CSRF protection: check state matches session
    if (state.empty() || !session->find("state") || session->get<std::string>("state") != state) {
        LOG_ERROR << "CSRF state mismatch or expired session. request_state=" << state 
                  << ", session_state=" << (session->find("state") ? session->get<std::string>("state") : "NOT_FOUND")
                  << ", session_id=" << session->sessionId();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("CSRF state mismatch or expired session");
        callback(resp);
        return;
    }

    std::string code_verifier = session->get<std::string>("code_verifier");

    // Make HttpClient pointing to the OIDC provider.
    // Set validation of self-signed SSL certs to false in dev environment.
    auto client = HttpClient::newHttpClient(OPENID_URL, nullptr, false, false);
    auto tokenReq = HttpRequest::newHttpRequest();
    tokenReq->setMethod(drogon::Post);
    tokenReq->setPath("/connect/token");
    tokenReq->setContentTypeCode(drogon::ContentType::CT_APPLICATION_X_FORM);

    std::string postBody = "grant_type=authorization_code"
                           "&code=" + code +
                           "&redirect_uri=" + REDIRECT_URI +
                           "&client_id=" + CLIENT_ID +
                           "&client_secret=" + CLIENT_SECRET +
                           "&code_verifier=" + code_verifier;
    tokenReq->setBody(postBody);

    client->sendRequest(tokenReq, [this, session, callback = std::move(callback)](ReqResult res, const HttpResponsePtr &resp) mutable {
        if (res != ReqResult::Ok || !resp) {
            auto errorResp = HttpResponse::newHttpResponse();
            errorResp->setStatusCode(k500InternalServerError);
            errorResp->setBody("Failed to reach OIDC token endpoint");
            callback(errorResp);
            return;
        }

        auto json = resp->getJsonObject();
        if (!json || !json->isMember("access_token")) {
            auto errorResp = HttpResponse::newHttpResponse();
            errorResp->setStatusCode(k400BadRequest);
            errorResp->setBody("Invalid token response: " + std::string(resp->getBody()));
            callback(errorResp);
            return;
        }

        std::string access_token = (*json)["access_token"].asString();

        // Fetch UserInfo
        auto userInfoClient = HttpClient::newHttpClient(OPENID_URL, nullptr, false, false);
        auto userInfoReq = HttpRequest::newHttpRequest();
        userInfoReq->setMethod(drogon::Get);
        userInfoReq->setPath("/connect/userinfo");
        userInfoReq->addHeader("Authorization", "Bearer " + access_token);

        userInfoClient->sendRequest(userInfoReq, [session, access_token, callback = std::move(callback)](ReqResult uRes, const HttpResponsePtr &uResp) mutable {
            if (uRes != ReqResult::Ok || !uResp) {
                auto errorResp = HttpResponse::newHttpResponse();
                errorResp->setStatusCode(k500InternalServerError);
                errorResp->setBody("Failed to reach UserInfo endpoint");
                callback(errorResp);
                return;
            }

            auto uJson = uResp->getJsonObject();
            if (!uJson) {
                auto errorResp = HttpResponse::newHttpResponse();
                errorResp->setStatusCode(k400BadRequest);
                errorResp->setBody("Invalid UserInfo response: " + std::string(uResp->getBody()));
                callback(errorResp);
                return;
            }

            Json::StreamWriterBuilder builder;
            std::string user_info_str = Json::writeString(builder, *uJson);
            std::string name = uJson->isMember("name") ? (*uJson)["name"].asString() : "";
            std::string email = uJson->isMember("email") ? (*uJson)["email"].asString() : "";

            // Save user details in session
            session->modify([access_token, user_info_str, name, email](drogon::Session::SessionMap &map) {
                map["user_authenticated"] = true;
                map["access_token"] = access_token;
                map["user_info"] = user_info_str;
                map["user_name"] = name;
                map["user_email"] = email;
            });

            // Redirect to frontend dashboard
            auto redirectResp = HttpResponse::newRedirectionResponse("http://localhost:5173/");
            callback(redirectResp);
        });
    });
}

void AuthController::dashboard(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto session = req->getSession();
    if (!session->find("user_authenticated") || !session->get<bool>("user_authenticated")) {
        auto resp = HttpResponse::newRedirectionResponse("/login");
        callback(resp);
        return;
    }

    std::string name = session->get<std::string>("user_name");
    std::string email = session->get<std::string>("user_email");

    std::string html = R"(
    <!DOCTYPE html>
    <html lang="es">
    <head>
        <meta charset="UTF-8">
        <title>AdminGas - Dashboard</title>
        <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600&display=swap" rel="stylesheet">
        <style>
            body {
                font-family: 'Outfit', sans-serif;
                background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
                color: #f8fafc;
                display: flex;
                align-items: center;
                justify-content: center;
                height: 100vh;
                margin: 0;
            }
            .card {
                background: rgba(255, 255, 255, 0.05);
                backdrop-filter: blur(10px);
                border: 1px solid rgba(255, 255, 255, 0.1);
                padding: 40px;
                border-radius: 20px;
                text-align: center;
                box-shadow: 0 10px 30px rgba(0,0,0,0.5);
                max-width: 400px;
                width: 100%;
            }
            h1 {
                font-size: 2rem;
                margin-bottom: 20px;
                color: #38bdf8;
            }
            p {
                font-size: 1.1rem;
                color: #cbd5e1;
                margin: 10px 0;
            }
            .btn {
                display: inline-block;
                margin-top: 30px;
                padding: 12px 24px;
                background: #0284c7;
                color: #fff;
                text-decoration: none;
                border-radius: 8px;
                font-weight: 600;
                transition: background 0.3s;
            }
            .btn:hover {
                background: #0369a1;
            }
        </style>
    </head>
    <body>
        <div class="card">
            <h1>Bienvenido a AdminGas</h1>
            <p><strong>Nombre:</strong> )" + name + R"(</p>
            <p><strong>Email:</strong> )" + email + R"(</p>
            <a href="/logout" class="btn">Cerrar Sesión</a>
        </div>
    </body>
    </html>
    )";

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(drogon::ContentType::CT_TEXT_HTML);
    resp->setBody(html);
    callback(resp);
}

void AuthController::logout(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto session = req->getSession();
    session->clear();

    std::string redirectUrl = OPENID_URL + "/connect/logout?post_logout_redirect_uri=http://localhost:5173/signout-callback-oidc";
    auto resp = HttpResponse::newRedirectionResponse(redirectUrl);
    callback(resp);
}

void AuthController::signoutCallback(const HttpRequestPtr &req,
                                     std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto resp = HttpResponse::newRedirectionResponse("http://localhost:5173/");
    callback(resp);
}

void AuthController::status(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto session = req->getSession();
    Json::Value ret;
    if (!session->find("user_authenticated") || !session->get<bool>("user_authenticated")) {
        ret["authenticated"] = false;
    } else {
        ret["authenticated"] = true;
        ret["name"] = session->get<std::string>("user_name");
        ret["email"] = session->get<std::string>("user_email");
    }
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

// Helper Implementations

std::string AuthController::generateRandomHex(size_t length)
{
    size_t numBytes = (length + 1) / 2;
    std::vector<unsigned char> buffer(numBytes);
    if (RAND_bytes(buffer.data(), numBytes) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    std::stringstream ss;
    for (unsigned char b : buffer) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return ss.str().substr(0, length);
}

std::string AuthController::sha256(const std::string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256Context;
    SHA256_Init(&sha256Context);
    SHA256_Update(&sha256Context, str.c_str(), str.size());
    SHA256_Final(hash, &sha256Context);
    
    return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

std::string AuthController::base64UrlEncode(const std::string &str)
{
    static const char base64_url_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";

    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : str) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_url_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(base64_url_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    return out;
}

Json::Value AuthController::parseJwtPayload(const std::string &jwt)
{
    Json::Value payload;
    size_t firstDot = jwt.find('.');
    if (firstDot == std::string::npos) return payload;
    size_t secondDot = jwt.find('.', firstDot + 1);
    if (secondDot == std::string::npos) return payload;
    
    std::string payloadB64 = jwt.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string decoded = base64UrlDecode(payloadB64);
    
    Json::Reader reader;
    reader.parse(decoded, payload);
    return payload;
}
