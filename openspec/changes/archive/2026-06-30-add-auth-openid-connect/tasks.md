## 1. Configuration & Infrastructure

- [x] 1.1 Update backend/api_server/config.json to enable sessions with a 3600-second timeout
- [x] 1.2 Add OIDC configurations (Provider URL, Client ID, Client Secret, Redirect URI) as constants in AuthController.h
- [x] 1.3 Ensure controllers/AuthController.cc and controllers/AuthController.h are registered in backend/api_server/CMakeLists.txt

## 2. Security & PKCE Helpers

- [x] 2.1 Implement random hex string generation function (for CSRF state and PKCE code_verifier) using OpenSSL
- [x] 2.2 Implement SHA256 hashing and Base64URL encoding utilities (for PKCE code_challenge)

## 3. AuthController Core Implementation

- [x] 3.1 Implement GET /login to initialize PKCE/state, set session variables, and redirect to OpenID Provider
- [x] 3.2 Implement GET /callback to validate the returned state parameter against the session
- [x] 3.3 Implement POST request to OpenID /connect/token using Drogon HttpClient to exchange the authorization code for tokens
- [x] 3.4 Implement GET request to OpenID /connect/userinfo using the access token to fetch user profile details
- [x] 3.5 Implement JWT payload decoder to read user details from the token response
- [x] 3.6 Implement GET /dashboard as a protected endpoint validating the session and returning user information
- [x] 3.7 Implement GET /logout to clear active user session variables

## 4. Verification & Testing

- [x] 4.1 Build and execute api_server to test the login redirect, PKCE flow, and callback exchange
- [x] 4.2 Verify access control (dashboard redirects unauthenticated users; dashboard shows user info for authenticated users)
- [x] 4.3 Verify session termination upon accessing /logout
