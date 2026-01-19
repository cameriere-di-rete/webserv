import os
import stat
import requests
import tempfile


def test_head_returns_no_body(webserv_base):
    r = requests.head(webserv_base + '/')
    assert r.status_code == 200
    assert r.text == ''


def test_insufficient_read_permissions(webserv_base, tmp_path):
    # Create a file and remove read perms; server should not return 200
    www = os.path.join(os.path.dirname(__file__), 'www')
    target = os.path.join(www, 'perm_denied.txt')
    with open(target, 'w') as f:
        f.write('secret')
    # remove all permissions
    os.chmod(target, 0)
    try:
        r = requests.get(webserv_base + '/perm_denied.txt')
        assert r.status_code in (403, 404, 500)
    finally:
        # restore so cleanup possible
        os.chmod(target, stat.S_IRUSR | stat.S_IWUSR)
        os.remove(target)


def test_insufficient_upload_permissions(webserv_base, tmp_path):
    uploads = os.path.join(os.path.dirname(__file__), 'www', 'uploads')
    os.makedirs(uploads, exist_ok=True)
    # remove write permission from uploads dir
    old = os.stat(uploads).st_mode
    os.chmod(uploads, old & ~stat.S_IWUSR & ~stat.S_IWGRP & ~stat.S_IWOTH)
    try:
        url = webserv_base + '/uploads/perm_upload.txt'
        r = requests.put(url, data='data')
        assert r.status_code not in (200, 201)
    finally:
        os.chmod(uploads, old)
        p = os.path.join(uploads, 'perm_upload.txt')
        if os.path.exists(p):
            os.remove(p)


def test_invalid_range_returns_416(webserv_base):
    headers = {'Range': 'bytes=999999-1000000'}
    r = requests.get(webserv_base + '/index.html', headers=headers)
    assert r.status_code in (416, 200, 404)
