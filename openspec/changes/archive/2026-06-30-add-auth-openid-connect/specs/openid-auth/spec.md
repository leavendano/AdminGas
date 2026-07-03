## ADDED Requirements

### Requirement: OpenID Connect Authorization Initiation
The system MUST provide a `/login` endpoint that initiates the OpenID Connect authorization code flow with PKCE and CSRF protection. The system SHALL generate a cryptographically secure 64-character hexadecimal `state` and `code_verifier`, compute a SHA256-based `code_challenge` encoded in Base64URL, store the `state` and `code_verifier` in the session, and redirect the client to the Identity Provider's authorize endpoint with the calculated parameters.

#### Scenario: Successful Login Initiation
- **WHEN** a user accesses the `/login` endpoint
- **THEN** the system generates `state` and `code_verifier`, saves them in the session, and redirects to the authorization URL with PKCE parameters

### Requirement: OpenID Connect Authorization Callback Handling
The system MUST provide a `/callback` endpoint that handles the redirect from the Identity Provider. The system SHALL validate the returned `state` against the `state` in the session. If valid, the system SHALL exchange the authorization `code` and the saved `code_verifier` for an access token via a POST request, fetch the user info from the provider using the access token, save the user info in the session, and redirect to the `/dashboard`.

#### Scenario: Valid Authorization Callback
- **WHEN** the callback endpoint is hit with a valid `state` and `code`
- **THEN** the system validates the state, exchanges the code with the verifier, fetches user info, establishes the authenticated session, and redirects to `/dashboard`

#### Scenario: Invalid State Callback
- **WHEN** the callback endpoint is hit with a `state` that does not match the session
- **THEN** the system returns an authorization error and does not establish a session

### Requirement: Protected Dashboard Access
The system MUST protect the `/dashboard` endpoint. The system SHALL verify if the user's session is authenticated. If the session is not authenticated, the system SHALL redirect the user to `/login`. If authenticated, the system SHALL display the user's information.

#### Scenario: Authenticated Dashboard Access
- **WHEN** an authenticated user accesses `/dashboard`
- **THEN** the system displays the dashboard with the user's name and email

#### Scenario: Unauthenticated Dashboard Access
- **WHEN** an unauthenticated user accesses `/dashboard`
- **THEN** the system redirects the user to `/login`

### Requirement: User Logout
The system MUST provide a `/logout` endpoint. The system SHALL clear all user authentication information and data from the active session.

#### Scenario: Successful Logout
- **WHEN** a user accesses `/logout`
- **THEN** the system clears the session and redirects to a logout confirmation page
