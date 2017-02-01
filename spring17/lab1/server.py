from socket import *;
from BaseHTTPServer import BaseHTTPRequestHandler
from StringIO import StringIO
import sys

#parsing HTTP request method
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
serverSocket = socket(AF_INET, SOCK_STREAM)
#checking user input
if len(sys.argv) < 2:
    print 'Usage: python server.py <port-number>'
    exit(1)
port = int(sys.argv[1])

#binding socket to current address
#try to bind address
try:
    serverSocket.bind(('',port))
except Exception as e:
    raise e
    print 'fail to bind'
    exit(1)

serverSocket.listen(1)
print 'Server is listening'

while True:
    #accepting connection from incoming client
    connectionSocket,addr = serverSocket.accept()
    #fork new thread/process to handle
    #receiving GET request
    recvBuffer = connectionSocket.recv(1024).decode()
    print recvBuffer
    
    #parse http request.
    request = HTTPRequest(recvBuffer)
    #if there is error
    if request.error_code is not None:                   # None  (check this first)
        print request.error_code
        print 'bad HTTP request'
        exit(1)                                          # exit
    
    #check if this is GET request
    if request.command != "GET":          # "GET"
        print "Cannot handle " + request.command + " request" 
        exit(1)

    print request.request_version                        # "HTTP/1.1"
    #if request.path is / then set it to 'index.html'
    if request.path == "/":
        request.path = "/index.html"
        print request.path
    #try to open file
    try:
        fileToOpen = open("upload" + request.path, "rb")
    except Exception as e:
        # raise e
        message =  'File may not exist.'
        print message
        connectionSocket.send(message)
        continue
    #start reading + sending file
    byteRead = fileToOpen.read(20);
    while len(byteRead) != 0:
        connectionSocket.send(byteRead)
        byteRead = fileToOpen.read(20)
    #close file
    fileToOpen.close()

    # print len(request.headers)     # 3
    # print request.headers.keys()   
    # print request.headers['host']  

    #type of Request
    #

    #check if request is wellformed or not
    #

    #closing connection Socket
    connectionSocket.close()