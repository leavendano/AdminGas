# Autenticación OpenID Connect

Este proyecto implementa autenticación OpenID Connect usando el flujo de código de autorización (Authorization Code Flow).

## Configuración

El sistema está configurado para conectarse con un servidor OpenID Connect con los siguientes parámetros:

- **URL del servidor OpenID**: `https://localhost:7001`
- **Client ID**: `facturacion`
- **Client Secret**: `Cfdi-49725`
- **Redirect URI**: `http://localhost:8080/callback`

### Modificar la configuración

Si necesitas cambiar estos parámetros, edita el archivo [controllers/AuthController.h](controllers/AuthController.h):

```cpp
// Configuración del OpenID Provider
const std::string OPENID_URL = "https://localhost:7001";
const std::string CLIENT_ID = "facturacion";
const std::string CLIENT_SECRET = "Cfdi-49725";
const std::string REDIRECT_URI = "http://localhost:8080/callback";
```

**NOTA**: Actualmente el cliente HTTP está configurado para NO usar HTTPS (línea 57 de AuthController.cc). Si tu servidor OpenID usa HTTPS, cambia el tercer parámetro a `true`:

```cpp
auto client = HttpClient::newHttpClient(OPENID_URL, 1, true); // true = HTTPS
```

## Endpoints disponibles

### 1. `/login` - Inicio de sesión
- **Método**: GET
- **Descripción**: Redirige al usuario al servidor OpenID Connect para autenticarse
- **Proceso**:
  1. Genera un estado aleatorio (CSRF protection)
  2. Lo guarda en la sesión del usuario
  3. Redirige al endpoint de autorización del proveedor OpenID

### 2. `/callback` - Callback de autorización
- **Método**: GET
- **Descripción**: Endpoint que recibe el código de autorización del servidor OpenID
- **Parámetros esperados**:
  - `code`: Código de autorización
  - `state`: Estado para validación CSRF
- **Proceso**:
  1. Valida el parámetro `state` contra el guardado en sesión
  2. Intercambia el `code` por un access token
  3. Obtiene información del usuario usando el access token
  4. Guarda la información del usuario en la sesión
  5. Redirige al dashboard

### 3. `/dashboard` - Dashboard protegido
- **Método**: GET
- **Descripción**: Página protegida que requiere autenticación
- **Requiere**: Usuario autenticado (redirige a `/login` si no está autenticado)
- **Muestra**: Información del usuario (nombre y email)

### 4. `/logout` - Cerrar sesión
- **Método**: GET
- **Descripción**: Limpia la sesión del usuario y muestra página de logout

## Flujo de autenticación (con PKCE)

```
1. Usuario visita /login
   ↓
2. Se genera code_verifier (64 caracteres hex) y code_challenge (SHA256 + Base64URL)
   ↓
3. Redirige a https://localhost:7001/connect/authorize
   (incluye code_challenge y code_challenge_method=S256)
   ↓
4. Usuario se autentica en el servidor OpenID
   ↓
5. Servidor redirige a /callback?code=XXX&state=YYY
   ↓
6. Aplicación intercambia código por token (POST a /connect/token)
   (incluye code_verifier para validación PKCE)
   ↓
7. Aplicación obtiene info del usuario (GET a /connect/userinfo)
   ↓
8. Guarda datos en sesión y redirige a /dashboard
```

## Seguridad

### PKCE (Proof Key for Code Exchange)
El sistema implementa PKCE (RFC 7636) para mayor seguridad:
- Se genera un `code_verifier` aleatorio de 64 caracteres hexadecimales
- Se calcula el `code_challenge` usando SHA256 y Base64 URL encoding
- El `code_challenge` se envía al servidor OpenID en la autorización
- El `code_verifier` se guarda en la sesión y se envía en el intercambio de token
- El servidor valida que el `code_verifier` corresponda al `code_challenge`
- Esto previene ataques de interceptación del código de autorización

### Protección CSRF
El sistema implementa protección contra ataques CSRF usando el parámetro `state`:
- Se genera un valor aleatorio de 64 caracteres hexadecimales
- Se guarda en la sesión del usuario
- Se envía al servidor OpenID
- Se valida cuando regresa en el callback

### Gestión de sesiones
- Las sesiones están habilitadas en [config.json](config.json)
- Timeout de sesión: 3600 segundos (1 hora)
- Los datos guardados en sesión incluyen:
  - `user_authenticated`: boolean
  - `access_token`: string
  - `user_info`: JSON string
  - `user_name`: string
  - `user_email`: string

### Validación de tokens JWT
El código incluye una función `parseJwtPayload()` para decodificar tokens JWT (sin validación de firma). Para producción, se recomienda implementar validación completa de firma.

## Compilación y ejecución

```bash
# Compilar
mkdir -p build && cd build && cmake .. && make

# Ejecutar
./build/demo
```

El servidor se iniciará en `http://localhost:5555`

## Prueba del sistema

1. Asegúrate de que tu servidor OpenID Connect está corriendo en `https://localhost:7001`
2. Inicia la aplicación: `./build/demo`
3. Abre un navegador y visita: `http://localhost:5555/login`
4. Serás redirigido al servidor OpenID para autenticarte
5. Después de autenticarte, serás redirigido de vuelta al dashboard

## Implementación técnica

### Archivos principales

- [controllers/AuthController.h](controllers/AuthController.h) - Definición del controlador
- [controllers/AuthController.cc](controllers/AuthController.cc) - Implementación del controlador
- [config.json](config.json) - Configuración de la aplicación (sesiones habilitadas)

### Dependencias

- **Drogon**: Framework web y cliente HTTP
- **OpenSSL**: Generación de números aleatorios y decodificación Base64
- **JsonCpp**: Parsing de respuestas JSON

### Cliente HTTP

La implementación usa el cliente HTTP nativo de Drogon (no requiere libcurl). Las peticiones HTTP son síncronas usando `std::promise` y `std::future` para esperar las respuestas.

## Mejoras futuras

1. **Validación de firma JWT**: Implementar validación completa de tokens JWT
2. **Refresh tokens**: Implementar renovación automática de tokens
3. **Single Logout**: Implementar logout en el servidor OpenID
4. **PKCE**: Implementar PKCE (Proof Key for Code Exchange) para mayor seguridad
5. **Verificación SSL**: Habilitar verificación de certificados SSL en producción
6. **Almacenamiento de sesiones**: Usar Redis para sesiones distribuidas
