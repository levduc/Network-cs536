from socket import *
from httplib import HTTPResponse
from StringIO import StringIO
import sys
#this is to manually processing httpresponse
class modifiedHTTPReponse():
    def __init__(self, response_str):
        self._file = StringIO(response_str)
    def makefile(self, *args, **kwargs):
        return self._file

# source = FakeSocket(http_response_str)
# response = HTTPResponse(source)
# response.begin()
# print "status:", response.status
# print "single header:", response.getheader('Content-Type')
# print "content:", response.read(len(http_response_str)) # the len here will give a 'big enough' value to read the whole content

#creating client socket
clientSocket = socket(AF_INET,SOCK_STREAM)

#checking user input
if len(sys.argv) < 4:
    print 'Usage: python client.py <hostname> <hostport> <filename>'
    exit(1)

#parsing user input
serverName = sys.argv[1]
serverPort = int(sys.argv[2])
fileName = sys.argv[3]
# change "/" to "index.html"
if fileName == "/":
    fileName = "index.html"

#try to connect to server
try:
    clientSocket.connect((serverName,serverPort))
except Exception as e:
    raise e
#create http request and try to send
httpRequest = "GET /" + fileName + " HTTP/1.0\r\nHost: "+gethostname()+"\r\n\r\n"
try:
    clientSocket.send(httpRequest.encode())
except Exception as e:
    clientSocket.close()
    raise e

#wait for response server
firstRes= clientSocket.recv(1024)
processedFirstRes = modifiedHTTPReponse(firstRes)
httpRes = HTTPResponse(processedFirstRes)
httpRes.begin()
if httpRes.status != 200:
    print 'Status code: '+ str(httpRes.status)+ '. Closing connection'
    clientSocket.close()
    exit(1)

#try to create file
try:
    file = open("Download/"+fileName, "wb")
    file.write(httpRes.read(len(firstRes)))
except Exception as e:
    print '[Error] Fail to create file. Closing connection' 
    clientSocket.close()

#appending to file
followRes = clientSocket.recv(1024)
while len(followRes) != 0:
    file.write(followRes)
    followRes = clientSocket.recv(1024)
print('download completed.')    
file.close()
clientSocket.close()