import subprocess
import random
import time
import filecmp
import struct
import os
import shutil
import string
from ftplib import FTP
import threading



port = 20000
directory = '/root/test'
server = subprocess.Popen(['./server', '-port', '%d' % port, '-root', directory], stdout=subprocess.PIPE)

time.sleep(1)

ftp = FTP()
# ftp.set_debuglevel(2)


# def test_list(ftp: FTP):
#     print("entering list")
#     print(ftp.retrlines('LIST ../test'))
#     print(ftp.retrlines('LIST ../'))

def test_pwd(ftp: FTP):
    print("entering pwd")
    print(ftp.pwd())
    
def test_cwd(ftp: FTP):
    print("entering cwd")
    print(ftp.cwd('test'))
    print(ftp.pwd())
    print(ftp.cwd('../'))
    print(ftp.pwd())
    
def test_mkd_rename_rmd(ftp: FTP):
    print("entering mkd")
    print(ftp.mkd('test1'))
    print(ftp.retrlines('LIST'))
    print(ftp.rename('test1', 'test2'))
    print(ftp.retrlines('LIST'))
    print(ftp.rmd('test2'))
    
# print ftp connect info
print(ftp.connect('localhost', port))
print(ftp.login())
print(ftp.getwelcome())
# test_list(ftp)
try:
    test_pwd(ftp)
    test_cwd(ftp)
    test_mkd_rename_rmd(ftp)
except Exception as e:
    print(e)
    time.sleep(3)


print(ftp.quit())
server.kill()
