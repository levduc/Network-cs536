from socket import *;
import sys;
from BaseHTTPServer import BaseHTTPRequestHandler
from StringIO import StringIO

class HTTPRequest(BaseHTTPRequestHandler):
    def __init__(self, request_text):
        self.rfile = StringIO(request_text)
        self.raw_requestline = self.rfile.readline()
        self.error_code = self.error_message = None
        self.parse_request()

    def send_error(self, code, message):
        self.error_code = code
        self.error_message = message

#creating the tcp socket
serverSocket = socket(AF_INET, SOCK_STREAM);

#checking user input
if len(sys.argv) < 2:
    print 'Usage: python server.py <port-number>'
    exit(1)

#binding socket to current address
host = gethostname();
port = int(sys.argv[1]);
#try to bind address

try:
    serverSocket.bind((host,port));
except Exception as e:
    raise e
    print 'fail to bind';
    exit(1);

serverSocket.listen(1);
print 'Server is listening';

while True:
    #accepting connection from incoming client
    connectionSocket,addr = serverSocket.accept();
    #fork new thread/process to handle
    #receiving GET request
    recvBuffer = connectionSocket.recv(1024).decode();
    print recvBuffer;
    #Parse http request.
    request = HTTPRequest(recvBuffer);
    print request.error_code       # None  (check this first)
    print request.command          # "GET"
    print request.path             # "/who/ken/trust.html"
    print request.request_version  # "HTTP/1.1"
    print len(request.headers)     # 3
    print request.headers.keys()   # ['accept-charset', 'host', 'accept']
    print request.headers['host']  # "cm.bell-labs.com"

    #type of Request

    #Check if request is wellformed or not
    #
    connectionSocket.close();