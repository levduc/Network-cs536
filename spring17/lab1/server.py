from socket import *;
from BaseHTTPServer import BaseHTTPRequestHandler
from StringIO import StringIO
import sys
import threading
import time

#parsing HTTP request class
class HTTPRequest(BaseHTTPRequestHandler):
    def __init__(self, request_text):
        self.rfile = StringIO(request_text)
        self.raw_requestline = self.rfile.readline()
        self.error_code = self.error_message = None
        self.parse_request()

    def send_error(self, code, message):
        self.error_code = code
        self.error_message = message

class myThread(threading.Thread):
    def __init__(self,threadID, connectionSocket):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.connectionSocket = connectionSocket
    def run(self):
        recvBuffer = self.connectionSocket.recv(1024).decode()
        print "============================Thread-"+str(self.threadID)+"=================================="
        print recvBuffer
        #parse http request.
        request = HTTPRequest(recvBuffer)
        if request.error_code is not None:                   # None  (check this first)
            print request.error_code
            print '[Error] Bad HTTP request. Closing connection.'
            #bad request response
            message = "HTTP/1.0 400 Bad Request \r\nConnection: close\r\n\r\n"
            self.connectionSocket.send(message)
            self.connectionSocket.close()
            exit(1)
            
        if request.command != "GET":                         # "GET"
            print "[Error] Cannot handle " + str(request.command) + " request" 
            self.connectionSocket.close()
            exit(1)
        # time.sleep(5)
        # print request.request_version + "\r\n"             # "HTTP/1.1"

        #if request.path is / then set it to '/index.html'
        if request.path == "/":
            request.path = "/index.html"

        #try to open file
        try:
            fileToOpen = open("Upload" + request.path, "rb")
        except Exception as e:
            print '[Error] Fail to open. Fail may not exist or be accessable.'
            message = 'HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n'
            self.connectionSocket.send(message)
            self.connectionSocket.close()
            exit(1)
        #sending file
        byteRead = 'HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n'+fileToOpen.read();
        self.connectionSocket.send(byteRead)
        #close file
        fileToOpen.close()
        print 'Done!'
        #closing connection Socket
        self.connectionSocket.close()
        print 'Exiting ' + str(self.threadID)

def main():
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
    serverSocket.listen(5)
    print 'Server is listening'
    threadID = 0
    while True:
        #accepting connection from incoming client
        connectionSocket,addr = serverSocket.accept()
        #fork new thread/process to handle
        thread = myThread(threadID,connectionSocket)
        thread.start()
        threadID += 1

        # receiving GET request
        # recvBuffer = connectionSocket.recv(1024).decode()
        # print "============================HTTP-Request=================================="
        # print recvBuffer
        # print "=========================================================================="
        # #parse http request.
        # request = HTTPRequest(recvBuffer)
        # # print len(request.headers)
        # # print request.headers.keys()
        # # print request.headers['host']
        # #if there is error
        # if request.error_code is not None:                   # None  (check this first)
        #     print request.error_code
        #     print '[Error] Bad HTTP request. Closing connection.'
        #     #bad request response
        #     message = "HTTP/1.0 400 Bad Request \r\nConnection: close\r\n\r\n"
        #     connectionSocket.send(message)
        #     connectionSocket.close()
        #     continue
        
        # #check if this is GET request
        # if request.command != "GET":                         # "GET"
        #     print "[Error] Cannot handle " + request.command + " request" 
        #     connectionSocket.close()
        #     continue        

        # # print request.request_version + "\r\n"             # "HTTP/1.1"
        # #if request.path is / then set it to '/index.html'
        # if request.path == "/":
        #     request.path = "/index.html"

        # #try to open file
        # try:
        #     fileToOpen = open("Upload" + request.path, "rb")
        # except Exception as e:
        #     print '[Error] Fail to open. Fail may not exist or be accessable.'
        #     message = 'HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n'
        #     connectionSocket.send(message)
        #     connectionSocket.close()
        #     continue
        # #sending file
        # byteRead = 'HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n'+fileToOpen.read();
        # connectionSocket.send(byteRead)
        # #close file
        # fileToOpen.close()
        # print 'Done!'
        # #closing connection Socket
        # connectionSocket.close()
        # #end of while loop
main()