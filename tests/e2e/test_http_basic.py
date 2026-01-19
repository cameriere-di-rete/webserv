"""
Basic HTTP tests for the webserv server.
"""
import requests


def test_get_root(webserv_base):
    """Test GET / returns 200."""
    r = requests.get(webserv_base + '/')
    assert r.status_code == 200


def test_get_index_html_explicit(webserv_base):
    """Test GET /index.html returns 200."""
    r = requests.get(webserv_base + '/index.html')
    assert r.status_code == 200
    assert 'html' in r.text.lower()


def test_get_nonexistent_returns_404(webserv_base):
    """Test GET /nonexistent returns 404."""
    r = requests.get(webserv_base + '/nonexistent.html')
    assert r.status_code == 404


def test_unknown_method_not_crash(webserv_base):
    """Test that unknown HTTP method doesn't crash server."""
    r = requests.request('FOO', webserv_base + '/')
    assert r.status_code >= 400


def test_head_request_no_body(webserv_base):
    """Test HEAD request returns headers without body."""
    r = requests.head(webserv_base + '/')
    assert r.status_code == 200
    assert len(r.content) == 0
    assert 'Content-Length' in r.headers or 'Transfer-Encoding' in r.headers


def test_404_custom_page(webserv_base):
    """Test that 404 returns custom error page."""
    r = requests.get(webserv_base + '/nonexistent')
    assert r.status_code == 404
    assert 'Custom 404 Page' in r.text or 'Error' in r.text or '404' in r.text
