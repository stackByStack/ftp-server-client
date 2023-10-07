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
def create_test_file(filename):
  f = open(filename, 'wb')
  for i in range(10000):
    data = struct.pack('d', random.random())
    f.write(data)
  f.close()

def port_down(ftp: FTP, directory):
    print("entering port_down")
    filename = 'test%d.data' % random.randint(100, 200)
    create_test_file(directory + '/' + filename)
    # ftp.set_pasv(False)
    if not ftp.retrbinary('RETR %s' % filename, open(filename, 'wb').write).startswith('226'):
      print('Bad response for RETR')
    #   credit -= minor
    ftp.set_pasv(True)
    if not filecmp.cmp(filename, directory + '/' + filename):
      print('Something wrong with RETR')
    #   credit -= major
    os.remove(directory + '/' + filename)
    os.remove(filename)
def pasv_upload(ftp: FTP, directory):
    print("entering pasv_load")
    ftp.set_pasv(True)
    filename = 'test%d.data' % random.randint(100, 200)
    create_test_file(filename)
    if not ftp.storbinary('STOR %s' % filename, open(filename, 'rb')).startswith('226'):
      print('Bad response for STOR')
      
    if not filecmp.cmp(filename, directory + '/' + filename):
      print('Something wrong with STOR')
    os.remove(directory + '/' + filename)
    os.remove(filename)
    
def list(ftp: FTP):
    print("entering list")
    print(ftp.retrlines('LIST'))


port = 20000
directory = '/root/test'
server = subprocess.Popen(['./server', '-port', '%d' % port, '-root', directory], stdout=subprocess.PIPE)

time.sleep(1)

ftp = FTP()
ftp.set_debuglevel(2)
# print ftp connect info
print(ftp.connect('localhost', port))
print(ftp.login())
print(ftp.getwelcome())
# use two thread to test
t1 = threading.Thread(target=port_down, args=(ftp, directory))
t2 = threading.Thread(target=pasv_upload, args=(ftp, directory))
t3 = threading.Thread(target=pasv_upload, args=(ftp, directory))
# wait for two thread
t1.start()
t2.start()
t3.start()

# test LIST
# print("main process")
# print(ftp.retrlines('LIST'))

t1.join()
t2.join()
t3.join()
# print ftp quit info
print(ftp.quit())
server.kill()
