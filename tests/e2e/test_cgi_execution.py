"""
CGI (Common Gateway Interface) tests for the webserv server.
"""
import requests


def test_cgi_get_basic(webserv_base):
    """Test basic CGI GET request."""
    r = requests.get(webserv_base + '/cgi-bin/basic.sh')
    assert r.status_code == 200
    assert len(r.text) > 0


def test_cgi_get_simple(webserv_base):
    """Test simple CGI GET request."""
    r = requests.get(webserv_base + '/cgi-bin/simple.sh')
    assert r.status_code == 200
    assert len(r.text) > 0
