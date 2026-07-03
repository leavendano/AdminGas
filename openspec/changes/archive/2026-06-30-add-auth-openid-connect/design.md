## Context

The AdminGas platform currently lacks an authentication mechanism. To secure the endpoints and dashboards, we will integrate OpenID Connect (OIDC) authentication using Drogon's backend, connecting to an external OpenID Provider (OP) at `https://localhost:7001`. The backend needs to handle authorization redirect, callback logic, token exchange with PKCE, user info retrieval, and session storage.

## Goals / Non-Goals

**Goals:**
- Implement `AuthController` to register `/login`, `/callback`, `/dashboard`, and `/logout` endpoints.
- Generate secure random `state` and `code_verifier` parameters.
- Compute PKCE SHA256-based `code_challenge` and request Authorization Code Flow.
- Validate `state` against session context to prevent CSRF.
- Intercambio (exchange) authorization code for ID and Access Tokens, and fetch user profile via UserInfo endpoint.
- Store user authentication state and metadata in the Drogon session.

**Non-Goals:**
- Full cryptographic signature validation of JWT (out of scope for this initial phase, basic payload parsing only).
- Multi-tenant OpenID Provider configuration.
- Integrating database persistence (Redis/Postgres) for sessions; standard Drogon in-memory sessions will be used.

## Decisions

### 1. Drogon Native HttpClient
- **Option A**: Use curl / libcurl for token/userInfo HTTP requests.
- **Option B (Chosen)**: Use Drogon's native `drogon::HttpClient`.
- **Rationale**: Drogon's native client avoids introducing external runtime and compile-time dependencies, integrates directly with the Drogon event loop, and simplifies testing. We will use synchronous promises/futures to wait for the OIDC provider's responses within endpoint handlers to keep the flow synchronous and simple.

### 2. Cryptographic and Hashing Operations
- **Option A**: Implement custom Base64URL and SHA256 utilities.
- **Option B (Chosen)**: Leverage OpenSSL functions (`SHA256`, `RAND_bytes`) and Drogon's built-in base64 helpers.
- **Rationale**: OpenSSL is already a dependency of Drogon and provides secure random number generation and standardized hash implementations.

### 3. Session Middleware and Configuration
- **Option A**: Implement custom cookie handling and validation.
- **Option B (Chosen)**: Enable Drogon's built-in session module via `config.json` with a 3600-second session timeout.
- **Rationale**: Leverage tested framework features rather than inventing session state validation mechanisms.

## Risks / Trade-offs

- **[Risk]** Self-signed SSL certificates on the identity provider (`https://localhost:7001`) cause HTTP client connection failure.
  - **Mitigation** → Configure the backend's `HttpClient` to temporarily bypass SSL verification (enable boolean third parameter) in development, and provide a configurable flag to enforce it in production.
- **[Risk]** Hardcoded client secret (`Zimat_1985`) in source files.
  - **Mitigation** → Store credentials as configuration constants in `AuthController.h` for initial implementation, with recommendations to load them via environment variables or `config.json` in production.
- **[Risk]** Session hijacking.
  - **Mitigation** → Use HTTPS for the client browser and configure Drogon's session settings (e.g., secure cookies) to minimize transport security risks.
