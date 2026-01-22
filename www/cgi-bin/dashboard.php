#!/usr/bin/env php-cgi
<?php
/**
 * Session dashboard - shows session info and protected content.
 */

header('Content-Type: text/html; charset=utf-8');

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

// Check authentication
$logged_in = isset($cookies['session_user']) && isset($cookies['session_id']);
$username = $logged_in ? urldecode($cookies['session_user']) : null;

?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Session Dashboard - webserv</title>
  <link rel="stylesheet" href="/css/style.css" />
  <style>
    body.test-page {
      padding: 2rem;
    }

    .auth-status {
      background: var(--card-bg);
      padding: 1.5rem;
      border-radius: var(--radius);
      margin-bottom: 2rem;
      border-left: 4px solid var(--accent);
    }

    .auth-status.not-authenticated {
      border-left-color: var(--err);
    }

    .auth-status h2 {
      margin: 0 0 0.5rem 0;
      border: none;
      font-size: 1.1rem;
    }

    .session-table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 1.5rem;
    }

    .session-table th,
    .session-table td {
      padding: 0.75rem;
      text-align: left;
      border-bottom: 1px solid var(--card-hover);
    }

    .session-table th {
      background: var(--panel-bg);
      color: var(--accent);
      font-weight: bold;
    }

    .session-table code {
      background: var(--panel-bg);
      padding: 0.2rem 0.4rem;
      border-radius: 3px;
      font-family: var(--font-mono);
      color: var(--accent);
    }

    .protected-content {
      background: var(--card-bg);
      padding: 1.5rem;
      border-radius: var(--radius);
      margin: 1.5rem 0;
    }

    .protected-content ol {
      margin: 1rem 0;
      padding-left: 1.5rem;
    }

    .action-buttons {
      display: flex;
      gap: 0.75rem;
      margin-top: 1.5rem;
      flex-wrap: wrap;
    }

    .action-buttons a {
      padding: 0.75rem 1.5rem;
      background: var(--btn-bg);
      color: var(--fg);
      text-decoration: none;
      border-radius: 6px;
      transition: background 0.2s;
    }

    .action-buttons a:hover {
      background: var(--btn-hover);
    }

    .action-buttons a.logout {
      background: var(--method-delete-bg);
      color: var(--method-delete-fg);
    }

    .action-buttons a.logout:hover {
      opacity: 0.9;
    }
  </style>
</head>
<body class="test-page">
  <div class="container">
    <a href="/" class="back-link">← Back to Home</a>

    <h1>🎛️ Session Dashboard</h1>
    <p class="subtitle">Session management and protected content demo</p>

    <?php if ($logged_in): ?>
      <div class="auth-status">
        <h2>✓ Authenticated</h2>
        <p>Welcome back, <strong><?php echo htmlspecialchars($username); ?></strong>!</p>
      </div>

      <h2>📋 Session Information</h2>
      <table class="session-table">
        <thead>
          <tr>
            <th>Cookie Name</th>
            <th>Value</th>
          </tr>
        </thead>
        <tbody>
          <?php foreach ($cookies as $name => $value): ?>
          <tr>
            <td><code><?php echo htmlspecialchars($name); ?></code></td>
            <td><code><?php echo htmlspecialchars(substr($value, 0, 40)) . (strlen($value) > 40 ? '...' : ''); ?></code></td>
          </tr>
          <?php endforeach; ?>
        </tbody>
      </table>

      <div class="protected-content">
        <h3>🔒 Protected Content</h3>
        <p>This section is only visible to authenticated users. Your session is maintained through HTTP cookies:</p>
        <ul>
          <li><code>session_id</code> - Unique session identifier (HttpOnly)</li>
          <li><code>session_user</code> - Your username</li>
          <li>Expires on logout or session timeout</li>
        </ul>
      </div>

      <div class="protected-content">
        <h3>🔍 How It Works</h3>
        <ol>
          <li>You submitted credentials via POST to <code>/cgi-bin/login.php</code></li>
          <li>Server validated your username and password</li>
          <li>Server generated a session ID with <code>md5(uniqid())</code></li>
          <li>Server sent <code>Set-Cookie</code> headers to your browser</li>
          <li>Browser stored the cookies automatically</li>
          <li>Each request now includes cookies in the <code>Cookie</code> header</li>
          <li>webserv passes cookies to PHP CGI via <code>HTTP_COOKIE</code> environment variable</li>
          <li>PHP script verifies both cookies are present before showing this page</li>
        </ol>
      </div>

    <?php else: ?>
      <div class="auth-status not-authenticated">
        <h2>✗ Not Authenticated</h2>
        <p>You need to log in to access this page and view protected content.</p>
      </div>

      <div class="protected-content">
        <h3>ℹ️ About This Demo</h3>
        <p>This page demonstrates cookie-based session management with PHP CGI running in webserv.</p>
        <p>It shows how:</p>
        <ul>
          <li>HTTP cookies are used to maintain user sessions</li>
          <li>webserv passes cookies to CGI scripts</li>
          <li>PHP scripts can authenticate users based on cookies</li>
          <li>Protected content is restricted to authenticated sessions</li>
        </ul>
      </div>
    <?php endif; ?>

    <div class="action-buttons">
      <?php if ($logged_in): ?>
        <a href="/cgi-bin/dashboard.php">🔄 Refresh</a>
        <a href="/cgi-bin/logout.php" class="logout">🚪 Logout</a>
      <?php else: ?>
        <a href="/cgi-bin/login.php">🔐 Login</a>
      <?php endif; ?>
    </div>
  </div>
</body>
</html>
