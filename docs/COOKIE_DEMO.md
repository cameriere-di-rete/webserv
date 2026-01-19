# 🍪 PHP Cookie Session Demo

Questa demo illustra il funzionamento dei cookie e della gestione delle sessioni in webserv con PHP CGI.

## 📋 Requisiti

- webserv compilato e in esecuzione
- `php-cgi` installato (versione 8.1+)
- Wrapper `/workspaces/webserv/run-php-cgi.sh` per gestire `force-cgi-redirect`

## 🚀 Come usare

### Avvia il server

```bash
cd /workspaces/webserv
./build/webserv conf/test-features.conf
```

### Accedi alla demo

Apri il browser e vai a:
```
http://localhost:8080/test-files/cookie_demo.html
```

## 📁 File della Demo

### Script PHP
- **[login.php](../../www/cgi-bin/login.php)** - Form di login con autenticazione
  - Test: admin/admin123, user/password, demo/demo
  - Imposta cookie di sessione: `session_user`, `session_id`

- **[logout.php](../../www/cgi-bin/logout.php)** - Logout e cancellazione cookie
  - Espira i cookie impostando `Expires=Thu, 01 Jan 1970`

- **[session_demo.php](../../www/cgi-bin/session_demo.php)** - Dashboard protetto
  - Mostra info di sessione se autenticato
  - Visualizza tutti i cookie ricevuti

- **[session_api.php](../../www/cgi-bin/session_api.php)** - API JSON
  - Endpoint per verifica stato autenticazione
  - Ritorna JSON: `{authenticated, username, session_id, timestamp, cookies_received}`

### Script Shell (CGI)
- **[echo_cookie.sh](../../www/cgi-bin/echo_cookie.sh)** - Stampa `HTTP_COOKIE`
  - Usa per testare il passaggio dei cookie da webserv

- **[setcookie.sh](../../www/cgi-bin/setcookie.sh)** - Imposta/cancella cookie
  - Query `?del=1` per cancellare il cookie

### HTML
- **[cookie_demo.html](../../www/test-files/cookie_demo.html)** - Pagina interattiva
  - Test shell script cookie
  - Test sessione PHP con AJAX
  - Design moderno e responsive

## 🔐 Come Funziona

### Flow di Login

1. **GET /cgi-bin/login.php** → Mostra form HTML
2. **POST /cgi-bin/login.php** con `username=admin&password=admin123`
   - PHP valida le credenziali
   - Genera `session_id = md5(uniqid())`
   - Risponde con `Set-Cookie: session_user=admin; Path=/`
   - Risponde con `Set-Cookie: session_id=<hash>; Path=/; HttpOnly`
3. **Browser** salva i cookie
4. **Successive richieste** includono i cookie nell'header `Cookie`

### Passaggio dei Cookie a CGI

1. Webserv riceve: `Cookie: session_user=admin; session_id=abc123`
2. CgiHandler.cpp estrae i cookie con `getHeaders("Cookie")`
3. Li unisce: `joined = "session_user=admin; session_id=abc123"`
4. Imposta: `setenv("HTTP_COOKIE", joined.c_str(), 1)`
5. PHP CGI legge: `$_SERVER['HTTP_COOKIE']`
6. Script PHP fa il parse e accede ai cookie

### Variabili d'Ambiente CGI

Webserv configura:
```
REQUEST_METHOD=GET/POST
REQUEST_URI=/cgi-bin/login.php
QUERY_STRING=...
CONTENT_TYPE=application/x-www-form-urlencoded
CONTENT_LENGTH=...
HTTP_COOKIE=session_user=admin; session_id=abc123
SERVER_NAME=webserv
SERVER_PORT=8080
SCRIPT_NAME=/cgi-bin/login.php
```

## 🛠️ Implementazione Tecnica

### Wrapper PHP-CGI

File: `/workspaces/webserv/run-php-cgi.sh`

```bash
#!/bin/sh
export REDIRECT_STATUS=200
if [ -z "$SCRIPT_FILENAME" ]; then
    export SCRIPT_FILENAME="$1"
fi
exec /usr/bin/php-cgi
```

**Perché?** PHP-CGI è compilato con `force-cgi-redirect=on` per sicurezza. Richiede:
- `REDIRECT_STATUS` per accettare richieste
- `SCRIPT_FILENAME` per trovare lo script

### Aggiunta dei Cookie in CgiHandler

File: `src/handlers/CgiHandler.cpp`

```cpp
// Export Cookie headers to HTTP_COOKIE environment variable for CGI.
std::vector<std::string> cookie_headers = conn.request.getHeaders("Cookie");
if (!cookie_headers.empty()) {
    std::string joined;
    for (auto it = cookie_headers.begin(); it != cookie_headers.end(); ++it) {
        if (!joined.empty()) {
            joined += "; ";
        }
        joined += *it;
    }
    setenv("HTTP_COOKIE", joined.c_str(), 1);
}
```

## 📊 Flowchart

```
┌─────────────────┐
│  Browser        │
│  (cookie_demo)  │
└────────┬────────┘
         │
         ├─→ GET /cgi-bin/login.php ──────────┐
         │                                      │
         │  ← HTML form                        │
         │                                      ▼
         │  POST /cgi-bin/login.php    ┌──────────────┐
         ├──────────────────────────→  │ webserv      │
         │  (username, password)       │              │
         │                             │ CgiHandler   │
         │                             │ (fork/exec)  │
         │                             └──────┬───────┘
         │                                    │
         │                                    ▼
         │                            ┌──────────────┐
         │  ← Set-Cookie ────────────┤ /run-php-    │
         │    session_user           │ cgi.sh       │
         │    session_id             │              │
         │                           │ (REDIRECT_   │
         │                           │  STATUS=200) │
         │                           │              │
         │                           └──────┬───────┘
         │                                  │
         │                                  ▼
         │                          ┌──────────────┐
         │                          │ login.php    │
         │                          │ (PHP script) │
         │                          │              │
         │                          │ ✓ Validate   │
         │                          │ ✓ Set-Cookie │
         │                          │ ✓ HTML resp  │
         │                          └──────────────┘
         │
         │ Cookie memorizzato
         │
         ├─→ GET /cgi-bin/session_api.php
         │   Cookie: session_user=...; session_id=...
         │
         │  webserv
         │  ├─ getHeaders("Cookie")
         │  ├─ setenv("HTTP_COOKIE", ...)
         │  └─ exec session_api.php
         │
         │  session_api.php legge HTTP_COOKIE
         │  ← JSON {authenticated: true, username: "admin"}
         │
         └─ Visualizza stato
```

## 🧪 Test Manuali

### Test 1: Cookie Base (Shell Script)
```bash
# Imposta un cookie
curl -b "test=value" http://localhost:8080/cgi-bin/echo_cookie.sh
# Output: test=value
```

### Test 2: Login PHP
```bash
# Login
curl -c cookies.txt -X POST \
  -d "username=admin&password=admin123" \
  http://localhost:8080/cgi-bin/login.php

# Usa i cookie
curl -b cookies.txt http://localhost:8080/cgi-bin/session_api.php
# Output: {"authenticated": true, "username": "admin", ...}
```

### Test 3: Logout
```bash
curl -b cookies.txt http://localhost:8080/cgi-bin/logout.php
# I cookie vengono cancellati (Expires nel passato)
```

## 🔒 Considerazioni di Sicurezza

1. **HttpOnly flag** - Protegge da XSS (cookie non accessibili via JS)
   ```php
   header('Set-Cookie: session_id=' . $id . '; Path=/; HttpOnly');
   ```

2. **Secure flag** - Dovresti aggiungere in produzione (HTTPS only)
   ```php
   header('Set-Cookie: session_id=' . $id . '; Path=/; HttpOnly; Secure');
   ```

3. **SameSite** - Protegge da CSRF (opzionale)
   ```php
   header('Set-Cookie: session_id=' . $id . '; Path=/; HttpOnly; SameSite=Strict');
   ```

4. **Session ID** - Dovrebbe essere:
   - Lungo e casuale
   - Regenerato dopo login
   - Invalidato dopo logout
   - Timeout dopo inattività

## 📝 Note

- La demo usa credenziali hardcoded. **MAI** fare questo in produzione!
- I cookie non sono crittografati. Usa HTTPS in produzione.
- Le sessioni non hanno timeout. Dovresti implementare TTL.
- I dati di sessione sono solo in memoria. Usa DB in produzione.

## 🎓 Cosa Impari

- Come webserv passa gli header HTTP alle CGI
- Come PHP CGI riceve e processa i cookie
- Come funzionano Set-Cookie e Cookie header
- Come implementare autenticazione con sessioni
- Come usare environment variables nelle CGI
- Il ruolo di REDIRECT_STATUS in PHP-CGI
