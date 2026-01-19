#!/workspaces/webserv/run-php-cgi.sh
<?php
/**
 * Simple PHP login demo using cookies for session management.
 * Accepts POST with username/password or GET to show login form.
 */

header('Content-Type: text/html; charset=utf-8');

// Simple user database (in real app, use proper DB)
$users = array(
    'admin' => 'admin123',
    'user' => 'password',
    'demo' => 'demo'
);

// Check if already logged in
$logged_in = false;
$current_user = '';
if (isset($_SERVER['HTTP_COOKIE'])) {
    $cookies = array();
    foreach (explode(';', $_SERVER['HTTP_COOKIE']) as $cookie) {
        $parts = explode('=', trim($cookie), 2);
        if (count($parts) === 2) {
            $cookies[$parts[0]] = $parts[1];
        }
    }
    if (isset($cookies['session_user'])) {
        $logged_in = true;
        $current_user = urldecode($cookies['session_user']);
    }
}

// Handle login POST
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Parse POST data from stdin
    $post_data = file_get_contents('php://input');
    if (empty($post_data)) {
        $post_data = file_get_contents('php://stdin');
    }
    $post = array();
    parse_str($post_data, $post);

    $username = isset($post['username']) ? $post['username'] : '';
    $password = isset($post['password']) ? $post['password'] : '';

    if (isset($users[$username]) && $users[$username] === $password) {
        // Successful login - set cookie
        $session_id = md5(uniqid($username, true));
        // Use replace=false to allow multiple Set-Cookie headers
        header('Set-Cookie: session_id=' . $session_id . '; Path=/; HttpOnly', false);
        header('Set-Cookie: session_user=' . urlencode($username) . '; Path=/', false);
        echo <<<'HTML'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Login Success - webserv</title>
  <link rel="stylesheet" href="/css/style.css" />
  <style>
    .nav-links {
      display: flex;
      gap: 0.75rem;
      margin-top: 1.5rem;
      flex-wrap: wrap;
    }

    .nav-links a {
      padding: 0.75rem 1.5rem;
      background: var(--btn-bg);
      color: var(--fg);
      text-decoration: none;
      border-radius: 6px;
      transition: background 0.2s;
      display: inline-block;
    }

    .nav-links a:hover {
      background: var(--btn-hover);
    }
  </style>
</head>
<body class="landing">
  <div class="site-root">
    <main class="content">
      <div class="box">
        <h1 class="title">✓ Login Successful</h1>
        <div class="by">Welcome, <strong>
HTML;
        echo htmlspecialchars($username);
        echo <<<'HTML'
</strong></div>
        <div class="flex nav-links" style="justify-content: center;">
          <a href="/cgi-bin/session_demo.php">📊 Go to Dashboard</a>
          <a href="/test-files/cookie_demo.html">🧪 Demo Tests</a>
        </div>
      </div>
    </main>
  </div>
</body>
</html>
HTML;
        exit;
    } else {
        // Failed login
        echo <<<'HTML'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Login Failed - webserv</title>
  <link rel="stylesheet" href="/css/style.css" />
</head>
<body class="landing">
  <div class="site-root">
    <main class="content">
      <div class="box">
        <h1 class="title">✗ Login Failed</h1>
        <div class="by" style="color: var(--err);">Invalid username or password.</div>
        <a href="/cgi-bin/login.php" class="nav-link">← Try again</a>
      </div>
    </main>
  </div>
</body>
</html>
HTML;
        exit;
    }
}

// Show login form (GET) or already logged in message
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Login - webserv</title>
  <link rel="stylesheet" href="/css/style.css" />
  <style>
    .form-box {
      background: var(--card-bg);
      padding: 2rem;
      border-radius: var(--radius);
      width: 100%;
      max-width: 350px;
    }

    .credentials-box {
      background: var(--panel-bg);
      padding: 1rem;
      border-left: 3px solid var(--accent);
      border-radius: 4px;
      margin-top: 1.5rem;
      font-size: 0.85rem;
    }

    .credentials-box strong {
      color: var(--accent);
      display: block;
      margin-bottom: 0.5rem;
    }

    .credentials-box code {
      display: block;
      color: var(--muted);
      margin: 0.25rem 0;
    }

    input[type="password"] {
      background: var(--bg);
      color: var(--fg);
      border: 1px solid var(--card-hover);
      padding: 0.5rem;
      border-radius: 4px;
      font-size: 1rem;
      width: 100%;
      box-sizing: border-box;
      margin-bottom: 1rem;
    }

    input[type="password"]:focus {
      outline: none;
      border-color: var(--accent);
    }
  </style>
</head>
<body class="landing">
  <div class="site-root">
    <main class="content">
      <div class="form-box">
        <?php if ($logged_in): ?>
          <h1 class="title" style="font-size: 1.5rem;">✓ Already Logged In</h1>
          <p style="color: var(--muted); margin: 1rem 0;">You are currently logged in as <strong><?php echo htmlspecialchars($current_user); ?></strong>.</p>
          <a href="/cgi-bin/session_demo.php" class="nav-link" style="margin-right: 0.5rem;">📊 Dashboard</a>
          <a href="/cgi-bin/logout.php" class="nav-link">🚪 Logout</a>
        <?php else: ?>
          <h1 class="title" style="font-size: 1.5rem; margin-bottom: 1.5rem;">🔐 Login</h1>

          <form method="POST" action="/cgi-bin/login.php">
            <label for="username">Username</label>
            <input type="text" id="username" name="username" required autofocus />

            <label for="password">Password</label>
            <input type="password" id="password" name="password" required />

            <button type="submit" style="width: 100%;">Sign In</button>
          </form>

          <div class="credentials-box">
            <strong>Test Credentials</strong>
            <code>admin / admin123</code>
            <code>user / password</code>
            <code>demo / demo</code>
          </div>
        <?php endif; ?>
      </div>
    </main>
  </div>
</body>
</html>
