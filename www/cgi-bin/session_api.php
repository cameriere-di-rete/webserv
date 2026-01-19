#!/workspaces/webserv/run-php-cgi.sh
<?php
/**
 * Simple JSON API endpoint to check session status.
 * Returns JSON with authentication status and user info.
 */

header('Content-Type: application/json');

// Parse cookies
$cookies = array();
if (isset($_SERVER['HTTP_COOKIE'])) {
    foreach (explode(';', $_SERVER['HTTP_COOKIE']) as $cookie) {
        $parts = explode('=', trim($cookie), 2);
        if (count($parts) === 2) {
            $cookies[$parts[0]] = $parts[1];
        }
    }
}

// Build response
$response = array(
    'authenticated' => false,
    'username' => null,
    'session_id' => null,
    'timestamp' => time(),
    'cookies_received' => array_keys($cookies)
);

if (isset($cookies['session_user']) && isset($cookies['session_id'])) {
    $response['authenticated'] = true;
    $response['username'] = urldecode($cookies['session_user']);
    $response['session_id'] = $cookies['session_id'];
}

echo json_encode($response, JSON_PRETTY_PRINT);
