"""
File operation tests for the webserv server.
"""
import requests

def test_delete_nonexistent_returns_404(webserv_base):
    """Test deleting a nonexistent file returns 404."""
    r = requests.delete(webserv_base + '/uploads/nonexistent.txt')
    assert r.status_code == 404
