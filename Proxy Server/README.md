README - The current file being read
Makefile - A file that contains all components to create the executable for the program myproxy
myproxy.c - A program that acts as a proxy for communication between a client and web server. When
the proxy is started on a listening port, a client can send HTTP requests and converts them to HTTPS 
requests to the proxy server. The proxy will then send it to the web server, where it will receive a response
and send it back to the client. The HTTP request can also be filtered based on a access control list.
Docu.txt - A document that lists 5 tests performed on the program for functionality and briefly
describes how the program works.