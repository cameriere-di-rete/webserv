import requests
from requests.exceptions import ReadTimeout


def test_get_root(webserv_base):
    r = requests.get(webserv_base + '/')
    assert r.status_code == 200


def test_unknown_method_not_crash(webserv_base):
    r = requests.request('FOO', webserv_base + '/')
    assert r.status_code >= 400


def test_404_custom_page(webserv_base):
    r = requests.get(webserv_base + '/nonexistent')
    assert r.status_code == 404
    assert 'Custom 404 Page' in r.text


def test_body_limit_enforced(webserv_base):
    long_body = 'this body is longer than the configured limit'
    r = requests.post(webserv_base + '/', data=long_body)
    assert r.status_code == 413


def test_ports_serving(webserv_base):
    # 8080 is covered by fixture; check 8081 too
    r = requests.get('http://127.0.0.1:8081/')
    assert r.status_code == 200


def test_cgi_hang_does_not_crash(webserv_base):
    # hang.cgi should time out quickly for the client but server must stay alive
    try:
        requests.get(webserv_base + '/cgi-bin/hang.cgi', timeout=2)
    except ReadTimeout:
        pass

    # server must still respond
    r = requests.get(webserv_base + '/')
    assert r.status_code == 200
