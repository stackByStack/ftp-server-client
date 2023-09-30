# Status code
| Code | Description |
| ---- | ----------- |
| 220  | Greeting          |
| 421 | Reject Greeting   |
| 500 | violate parsing rules      |
| 501 | inappropriate format of the parameter     |

## 220 Greeting
Accept connection.

## 421 Reject Greeting
Reject connection.

#  Miscellanies
[Reference](http://cr.yp.to/ftp.html)
## 1. Request ending with 
A request is a string of bytes. It contains a verb consisting of alphabetic ASCII characters;
optionally, a space followed by a parameter; and \015\012.
Some clients fail to include the \015, so I recommend that servers look only for \012. The parameter cannot contain \012.

## 2. Spaces between params
RFC 959 states in the text that multiple spaces are allowed before the parameter, but it states in the formal syntax that only one space is allowed before the parameter.

## 3. Case of Verb
RFC 959 specified that all verbs are interpreted without regard to case; so RETR and retr and Retr and rEtR have the same meaning.

## 4. Response format
The server's response consists of one or more lines. Each line is terminated by \012.

## 5. multiline response
Some clients are unable to handle responses longer than one line to various requests, even though RFC 959 permits multiple-line responses under most circumstances. I recommend that servers use one-line responses whenever possible. 

## 6. Reject Code 
The server can reject any request with code

- 421 if the server is about to close the connection;
- 500, 501, 502, or 504 for unacceptable syntax; or
- 530 if permission is denied.

Typically 500 means that the request violated some internal parsing rule in the server, 501 means that the server does not like the format of the parameter, 502 means that the server recognized the verb but does not support it, and 504 means that the server supports the verb but does not support the parameter.

## 7. pipeline block rules
- RFC 959 generally requires that the client wait for an accepting or rejecting response before sending the next request.
- Exceptions: RFC 959 allows the client to send a QUIT request, an ABOR request, or a STAT request without waiting for a response to a previous file transfer request (RETR, NLST, LIST, STOR, APPE, or STOU). 

## 8. Pathnames and encoded pathnames
A pathname is any string of bytes beginning with a slash and not containing \000.

An encoded pathname is a string of bytes not containing \012. It normally represents the pathname obtained by replacing each \000 in the encoded pathname with \012. However, if it does not start with a slash, it represents the pathname obtained by concatenating
- the server's current name prefix;
- a slash, if the name prefix does not end with a slash; and
- the string obtained by replacing each \000 in the encoded pathname with \012.

# TODO

- [ ] socketConn.h socket_recv_data modify memset to a more efficient way.