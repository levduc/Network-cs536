from socket import *;
serverName = 'diamondfromash'
serverPort = 12001

#creating client socket
clientSocket = socket(AF_INET,SOCK_STREAM)

#connecting to server
try:
	clientSocket.connect((serverName,serverPort))
except Exception as e:
	raise e

message = raw_input('Type your message:')

clientSocket.send(message.encode())

clientSocket.close()