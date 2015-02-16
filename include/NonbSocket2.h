/***************************************
 * Functions to implement SOCKET operations
 * Author: Lubing Xu
 * Date:   Nov. 1999
 ***************************************/
#ifndef _NONB_SOCKET2_H
#define _NONB_SOCKET2_H

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define SOCKET_CLOSED  -2
#define SOCKET_TIMEOUT -3

#ifdef __cplusplus
extern "C"{
#endif

#ifdef WIN32
void StartWinsock();
#endif

/*create a server socket listening on port*/
/***************************************
 * To create a TCP server socket listening
 * on the given port
 * Parameter:
 *  port: the port to listen at
 * Return:
 *  If the server socket is created
 * successfully, the socket handle
 * will be returned.
 *  INVALID_SOCKET otherwise.
 ***************************************/
int SocketServer(int port);

/********************************************
 * accept a connection from one client
 * Parameter:
 *  serversock: the socket handle of the listening server
 * Return:
 *  the client socket handle or INVALID_SOCKET
 * NOTE:
 *  If you want to know the information of client IP & port,
 *  this function doesn't provide them. Please use function
 *  accept() directly.
 ********************************************/
int Accept(int serversock);

/********************************************
 * Create a TCP client socket
 * Parameters:
 *  server: server name or IP
 *  prot:   server port
 * Return:
 *  INVALID_SOCKET: any error occurrs
 *  a socket handle: successful
 ********************************************/
int Socket(const char* server, int port);

/********************************************
 * Create a non-block TCP client socket
 * Parameters:
 *  server: server name or IP
 *  port:   server port
 *  timeout:seconds to wait for connection
 * Return:
 *  INVALID_SOCKET: any error occurrs
 *  a socket handle: successful
 ********************************************/
int NonbSocket(const char* server, int port, int timeout);

/***************************************
 * Send specified length of bytes
 * Parameters:
 *  sock: the socket handle to send
 *  buffer: the bytes to send
 *  total: the total bytes to send
 *  timeout: timeout int senconds
 * Return:
 *  >=0: the number of bytes sent if successful,
 *  SOCKET_TIMEOUT: timeout for waiting
 *  SOCKET_ERROR: if any error occurrs.
 ***************************************/
int Send(int sock, const char *buffer, int total, int timeout);

/***************************************
 * Receive bytes from a socket
 * Parameters:
 *  sock: the socket handle to receive
 *  buffer: the buffer to store the data
 *  size: the size of the buffer
 *  timeout: timeout in seconds
 * Return:
 *  >0: the number of bytes received successfully
 *  SOCKET_CLOSED: the socket has been closed
 *  SOCKET_TIMEOUT: timeout for waiting.
 *  SOCKET_ERROR: if any error occurrs
 ***************************************/
int Receive(int sock, char *buffer, int size, int timeout);

/***************************************
 * Send a string then send an extra new line
 * character
 * Parameters:
 *  sock: the socket handle to send
 *  buffer: a string ended with ASCIIZ
 *  timeout: timeout in seconds
 * Return:
 *  >0: if successfuly.
 *  SOCKET_ERROR: if any error occurrs.
 ***************************************/
int SendLine(int sock, const char *buffer, int timeout);

/***************************************
 * Read a line ended with new line char
 * Parameters:
 *  sock: the socket handle to read
 *  buffer: a buffer to store data
 *  size: the size of the buffer
 *  timeout: timeout in seconds
 * Return:
 *  >0: the length of line
 *  SOCKET_CLOSED: the socket has been closed
 *  SOCKET_TIMEOUT: timeout for waiting.
 *  SOCKET_ERROR: if any error occurrs
 * NOTE:
 *  any '\r' is ignored
 ***************************************/
int ReadLine(int sock, char *buffer, int size, int timeout);

/********************************************
 * Close a socket
 * Parameter:
 *  the socket handle to close
 ********************************************/
void CloseSocket(int sock);

#ifdef __cplusplus
}
#endif

#endif

