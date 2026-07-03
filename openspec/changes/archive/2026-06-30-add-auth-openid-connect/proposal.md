## Why

AdminGas needs to secure its admin panel using an enterprise identity provider. Implementing OpenID Connect (OIDC) authentication using the Authorization Code Flow with PKCE and CSRF protection secures access to admin dashboards, ensuring only authorized personnel can access or modify system state.

## What Changes

- **OIDC Configuration**: Add config settings for the identity provider (URL, Client ID, Client Secret, Redirect URI).
- **Authentication Endpoints**: Implement `/login`, `/callback`, `/dashboard` (protected), and `/logout` endpoints in the C++ Drogon backend.
- **Session Management**: Enable cookie/session state management in the Drogon server to keep track of authenticated users and session timeouts (1 hour).
- **Security Enhancements**: 
  - Proof Key for Code Exchange (PKCE) to protect the authorization code exchange.
  - CSRF protection using standard `state` validation.
  - Basic JWT payload parsing for user details.

## Capabilities

### New Capabilities
- `openid-auth`: Covers authentication endpoints, PKCE validation, state-based CSRF protection, and session management for AdminGas.

### Modified Capabilities
<!-- No modified capabilities -->

## Impact

- **Backend**: Adds `AuthController` (declaring routes for `/login`, `/callback`, `/dashboard`, `/logout`), configures openssl & jsoncpp dependencies, and updates the Drogon `config.json` to enable sessions.
- **Frontend**: Navigation layout and page routers will check session states and redirect unauthenticated users to `/login`.
