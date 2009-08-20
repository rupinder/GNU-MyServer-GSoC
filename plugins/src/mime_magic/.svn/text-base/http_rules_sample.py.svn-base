from http_checker import *

# This simple rule returns a 404 error page to every request from the
# host 192.168.0.2.
if get_remote_addr() == "192.168.0.2":
    raise_error(404)
