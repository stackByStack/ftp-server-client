import sys
import socket
from socket import _GLOBAL_DEFAULT_TIMEOUT
import re

FTP_PORT = 21
MAXLINE = 4096

# Exception raised when an error or invalid response is received
class Error(Exception): pass
class error_reply(Error): pass          # unexpected [123]xx reply
class error_temp(Error): pass           # 4xx errors
class error_perm(Error): pass           # 5xx errors
class error_proto(Error): pass          # response does not begin with [1-5]

#define error set
all_errors = (Error, EOFError, OSError)

CRLF = '\r\n'
bCRLF = b'\r\n'

class FTP: 
    debugLevel = 0
    host = ""
    port = FTP_PORT
    maxline = MAXLINE
    sock = None
    file = None
    welcome = None
    passiveserver = True
    
    def __init__(self, host='', user='', passwd='', acct='', timeout=_GLOBAL_DEFAULT_TIMEOUT, source_addr=None, encoding='utf-8'):
        self.encoding = encoding
        self.timeout = timeout
        self.source_addr = source_addr
        if host: # host is given, connect now
            self.connect(host)
            if(user):
                self.login(user, passwd, acct)
        
    # close connection        
    def __exit__(self, *args):
        if self.sock is not None: # close the socket
            try: 
                self.quit()
            except all_errors:
                pass
            finally:
                self.close()
    
    def connect(self, host='', port=0, timeout=-999):
        """
        Establish a connection to the specified host and port with an optional timeout.

        :param host: The hostname or IP address to connect to. Default is an empty string.
        :type host: str

        :param port: The port number to connect to. Default is 0.
        :type port: int

        :param timeout: The timeout for the connection. Default is -999, indicating no specific timeout.
        :type timeout: int

        :return: A welcome message or response from the server after establishing the connection.
        :rtype: str
        """
        if host != '':
            self.host = host
        if port > 0:
            self.port = port
        if timeout != -999:
            self.timeout = timeout
        self.sock = socket.create_connection((self.host, self.port), self.timeout, self.source_addr)
        self.af = self.sock.family
        self.file = self.sock.makefile('r', encoding=self.encoding)
        self.welcome = self.getresp()
        return self.welcome
    
    def getWelcome(self):
        return self.welcome
    
    def set_debuglevel(self, level):
        self.debugLevel = level
        
    def set_pasv(self, val):
        self.passiveserver = val
        
    def putline(self, line):
        """
        Send a line of text to the server after checking for illegal newline characters.

        :param line: The line of text to be sent to the server.
        :type line: str

        :raises ValueError: If the 'line' parameter contains illegal newline characters ('\\r' or '\\n').

        If the debug level is greater than 0, it prints the line before sending it to the server.

        :return: None
        """
        if '\r' in line or '\n' in line:
            raise ValueError('an illegal newline character should not be contained')
        line = line + CRLF
        if self.debugLevel > 0:
            print('*put*', line)
        self.sock.sendall(line.encode(self.encoding))
        
    def putcmd(self, line):
        if self.debugLevel > 0:
            print('*cmd*', line)
        self.putline(line)
    
    def getline(self):
        line = self.file.readline(self.maxline + 1)
        if len(line) > self.maxline:
            raise Error('got more than {} bytes'.format(self.maxline))
        if self.debugLevel > 0:
            print('*get*', line)
        if not line:
            raise EOFError
        if line[-2:] == CRLF:
            line = line[:-2]
        elif line[-1:] in CRLF:
            line = line[:-1]
        return line
    
    def getmultiline(self):
        line = self.getline()
        if line[3:4] == '-':
            code = line[:3]
            while 1:
                nextline = self.getline()
                line = line + ('\n' + nextline)
                if nextline[:3] == code and nextline[3:4] != '-':
                    break
        return line
    
    def getresp(self):
        """
        Get a response from the server and handle it accordingly.

        :raises error_temp: If the response code starts with '4' (temporary error).
        :raises error_perm: If the response code starts with '5' (permanent error).
        :raises error_proto: If the response code does not match any expected patterns.

        :return: The server response.
        :rtype: str
        """
        resp = self.getmultiline()
        self.lastresp = resp[:3]
        c = resp[:1]
        if c in ('1', '2', '3'):
            return resp
        if c == '4':
            raise error_temp(resp)
        if c == '5':
            raise error_perm(resp)
        raise error_proto(resp)
    
    def voidresp(self):
        resp = self.getresp()
        if resp[:1] != '2':
            raise error_reply(resp)
        return resp
    
    def sendcmd(self, cmd):
        self.putcmd(cmd)
        return self.getresp()
    
    def voidcmd(self, cmd):
        self.putcmd(cmd)
        return self.voidresp()
    
    def sendport(self, host, port):
        """
        Send a PORT command to the server, specifying the host and port for data transfer.

        :param host: The host (in the format 'A.B.C.D') to which data should be sent.
        :type host: str

        :param port: The port to use for data transfer.
        :type port: int

        :return: The response received from the server after sending the PORT command.
        :rtype: str
        """
        hbytes = host.split('.') # split into octets
        pbytes = [repr(port//256), repr(port%256)] # high, low byte
        bytes = ','.join(hbytes + pbytes) # make it a string
        cmd = 'PORT ' + bytes # send the port command
        return self.voidcmd(cmd) # expect a response
    
    def makeport(self):
        """
        create a new socket and send a PORT command for it
        """ 
        sock = socket.create_server(("", 0), family=self.af, backlog = 1); # backlog: the maximum number of queued connections
        port = sock.getsockname()[1] # get proper port
        host = self.sock.getsockname()[0]
        self.sendport(host, port)
        sock.settimeout(self.timeout)
        return sock
    
    def makepasv(self): 
        host, port = self.parse227(self.sendcmd('PASV'))
        return host, port
    
    def parse227(self, resp):
        # parse the PASV response for host and port information
        if resp[:3] != '227':
            raise error_reply(resp)
        
        _227_re = re.compile(r'(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
        m = _227_re.search(resp)
        if not m:
            raise error_proto(resp)
        nums = m.groups()
        host = '.'.join(nums[:4])
        port = (int(nums[4]) << 8) + int(nums[5])
        return host, port
    
    def ntransfercmd(self, cmd, rest=None):
        """
        Prepare for a data transfer command and return the connection and size information.

        :param cmd: The data transfer command (e.g., STOR, RETR).
        :type cmd: str

        :param rest: The restart marker for the data transfer. Default is None.
        :type rest: str or None

        :return: A tuple containing the connection and the size information.
        :rtype: (socket.socket, int or None)
        :raises error_reply: If the response from the server indicates an error.
        """
        size = None
        if self.passiveserver:
            host, port = self.makepasv()
            conn = socket.create_connection((host, port), self.timeout, self.source_addr)
            try:
                if rest is not None:
                    self.sendcmd("REST %s" % rest)
                resp = self.sendcmd(cmd)
                if resp[0] == '2':
                    resp = self.getresp()
                if resp[0] != '1':
                    raise error_reply(resp)
            except:
                conn.close()
                raise
        else:
            with self.makeport() as sock:
                if rest is not None:
                    self.sendcmd("REST %s" % rest)
                resp = self.sendcmd(cmd)
                if resp[0] == '2':
                    resp = self.getresp()
                if resp[0] != '1':
                    raise error_reply(resp)
                conn, sockaddr = sock.accept()
                if self.timeout is not _GLOBAL_DEFAULT_TIMEOUT:
                    conn.settimeout(self.timeout)
            
        if resp[:3] == '150':
            size = self.parse150(resp)
        return conn, size
    
    def parse150(self, resp):
        # return the expected transfer size from a PASV response
        if resp[:3] != '150':
            raise error_reply(resp)
        
        _150_re = re.compile(r'150 Opening .* \((\d+) bytes\)')
        m = _150_re.search(resp)
        if not m:
            return None
        return int(m.group(1))
    
    def transfercmd(self, cmd, rest=None):
        return self.ntransfercmd(cmd, rest)[0]
    
    def login(self, user = '', passwd = '', acct = ''):
        """
        Log in to the FTP server with the provided user, password, and account.

        :param user: The username for authentication. Default is 'anonymous'.
        :type user: str

        :param passwd: The password for authentication. Default is an empty string.
        :type passwd: str

        :param acct: The account information for authentication. Default is an empty string.
        :type acct: str

        :return: The response received from the server after logging in.
        :rtype: str
        :raises error_reply: If the response from the server indicates an error.
        """
        if not user:
            user = 'anonymous'
        if not passwd:
            passwd = ''
        if not acct:
            acct = ''
        if user == 'anonymous' and passwd in ('', '-'):
            passwd = passwd + 'anonymous@'
        resp = self.sendcmd('USER ' + user)
        self.username = user
        self.password = passwd
        if resp[0] == '3':
            resp = self.sendcmd('PASS ' + passwd)
        if resp[0] == '3':
            resp = self.sendcmd('ACCT ' + acct)
        if resp[0] != '2':
            raise error_reply(resp)
        return resp
    
    def retrbinary(self, cmd, callback, blocksize=8192, rest=None):
        self.voidcmd('TYPE I')
        with self.transfercmd(cmd, rest) as conn:
            while 1:
                data = conn.recv(blocksize)
                if not data:
                    break
                callback(data)
            conn.close()
        return self.voidresp()
    
    #remained to do : properly design callback function
    def retrlines(self, cmd, callback = None):
        if callback is None:
            callback = print
        resp = self.sendcmd('TYPE A')
        with self.transfercmd(cmd) as conn:
            fp = conn.makefile('r', encoding=self.encoding)
            while 1:
                line = fp.readline(self.maxline + 1)
                if len(line) > self.maxline:
                    raise Error('got more than {} bytes'.format(self.maxline))
                if self.debugLevel > 0:
                    print('*retr*', repr(line))
                if not line:
                    break
                if line[-2:] == CRLF:
                    line = line[:-2]
                elif line[-1:] in CRLF:
                    line = line[:-1]
                callback(line)
            fp.close()
            conn.close()
        return self.voidresp()
    
    def storbinary(self, cmd, fp, blocksize=8192, callback=None, rest=None):
        self.voidcmd('TYPE I')
        with self.transfercmd(cmd, rest) as conn:
            while 1:
                buf = fp.read(blocksize)
                if not buf:
                    break
                conn.sendall(buf)
                if callback:
                    callback(buf)
            conn.close()
        return self.voidresp()
    
    def acct(self, password):
        cmd = 'ACCT ' + password
        return self.voidcmd(cmd)
    
    def dir(self, *args):
        cmd = 'LIST'
        callback = None
        if args[-1:] and type(args[-1]) != type(''):
            args, callback = args[:-1], args[-1]
            
        for arg in args:
            cmd = cmd + (' ' + arg)
            
        print(cmd)
        return self.retrlines(cmd, callback)
        
    def rename(self, fromname, toname):
        resp = self.sendcmd('RNFR ' + fromname)
        if resp[0] != '3':
            raise error_reply(resp)
        return self.voidcmd('RNTO ' + toname)
    
    def delete(self, filename):
        resp = self.sendcmd('DELE ' + filename)
        if resp[:3] in ('250', '200'):
            return resp
        else:
            raise error_reply(resp)
        
    def cwd(self, dirname):
        if dirname == '..':
            try:
                return self.voidcmd('CDUP')
            except error_perm as msg:
                if msg.args[0][:3] != '500':
                    raise
        elif dirname == '':
            dirname = '.'
        cmd = 'CWD ' + dirname
        return self.voidcmd(cmd)
    
    def mkd(self, dirname):
        resp = self.voidcmd('MKD ' + dirname)
        return self.parse257(resp)
    
    def parse257(self, resp):
        if resp[:3] != '257':
            raise error_reply(resp)
        if resp[3:5] != ' "':
            return ''
        i = resp.find('"', 5)
        if i < 0:
            return ''
        j = resp.find('"', i+1)
        if j < 0:
            return ''
        return resp[i+1:j]
    
    def rmd(self, dirname):
        return self.voidcmd('RMD ' + dirname)
    
    def pwd(self):
        resp = self.voidcmd('PWD')
        if not resp.startswith('257'):
            raise error_reply(resp)
        return self.parse257(resp)
    
    def quit(self):
        resp = self.voidcmd('QUIT')
        self.close()
        return resp
    
    def close(self):
        if self.file is not None:
            self.file.close()
            self.file = None
        if self.sock is not None:
            self.sock.close()
            self.sock = None
    def delete(self, filename):
        resp = self.sendcmd('DELE ' + filename)
        if resp[:3] in ('250', '200'):
            return resp
        else:
            raise error_reply(resp)
    def syst(self):
        resp = self.sendcmd('SYST')
        if resp[:3] == '215':
            return resp[4:].strip()
        else:
            raise error_reply(resp)