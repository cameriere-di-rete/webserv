#!/usr/bin/env php-cgi
<?php
/**
 * Logout script - expires cookies to end the session.
 */

header('Content-Type: text/html; charset=utf-8');

// Expire the session cookies
header('Set-Cookie: session_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly', false);
header('Set-Cookie: session_user=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly', false);

?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Logged Out - webserv</title>
  <link rel="stylesheet" href="/css/style.css" />
</head>
<body class="landing">
  <div class="site-root">
    <main class="content">
      <div class="box">
        <h1 class="title">👋 Logged Out</h1>
        <div class="by">Your session has been cleared.</div>
        <a href="/cgi-bin/login.php" class="nav-link">← Back to Login</a>
      </div>
    </main>
  </div>
</body>
</html>
