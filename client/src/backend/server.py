from flask import Flask, request, jsonify, send_file
import uuid
from typing import Dict
from ftp import FTP
import io
from flask_cors import CORS

app = Flask(__name__) # create a Flask app
CORS(app) # allow cross origin


ftpSessions: Dict[str, FTP] = {} # sessionId -> FTP

@app.route('/connect', methods=['POST'])
def make_connection():
    global ftpSessions
    data = request.get_json()
    host = data['host']
    port = int(data['port'])
    ftp = FTP()
    resp = ftp.connect(host, port)
    if resp == '':
        # failed to connect
        return jsonify({'status': 'failed', 'desc': 'failed to connect'}), 400
    else :
        # connected
        sessionId = str(uuid.uuid4())
        ftpSessions[sessionId] = ftp
        return jsonify({'status': 'success', 'sessionId': sessionId}), 200
    
@app.route('/login', methods=['POST'])
def login():
    global ftpSessions
    data = request.get_json()
    print(data)
    
    
    username = data['username']
    password = data['password']
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.login(username, password)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to login'}), 400
    
@app.route('/setpassive', methods=['POST'])
def setpassive():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    pasv = data['pasv']
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.set_pasv(pasv)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to set passive'}), 400
    
@app.route('/dir', methods=['POST'])
def dir():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = data['path']
    ftp = ftpSessions[sessionId]
    try:
        result = []
        def callback(line):
            result.append(line)
        resp = ftp.dir(path, callback)
        return jsonify({'status': 'success', 'desc': resp, 'result': result}), 200
    except Exception as e:
        return jsonify({'status': 'failed', 'desc': e}), 400
    
@app.route('/download', methods=['POST'])
def download():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = data['path']
    originalFtp = ftpSessions[sessionId]
    newFtp = FTP()
    newFtp.connect(originalFtp.host, originalFtp.port)
    newFtp.login(originalFtp.username, originalFtp.password)
    try:
        buffer = []
        def callback(data: bytes):
            buffer.append(data)
        resp = newFtp.retrbinary('RETR ' + path, callback)
        fileStream = io.BytesIO(b''.join(buffer))
        filename = path.split('/')[-1]
        return send_file(fileStream, download_name=filename, as_attachment=True)
    except Exception as e:
        return jsonify({'status': 'failed', 'desc': e}), 400
    finally:
        newFtp.quit()
    
@app.route('/upload', methods=['POST'])
def upload():
    global ftpSessions
    sessionId = request.form['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = request.form['path']
    content = request.files['file']
    if path[-1] != '/':
        path += '/'
    orignalFtp = ftpSessions[sessionId]
    newFtp = FTP()
    newFtp.connect(orignalFtp.host, orignalFtp.port)
    newFtp.login(orignalFtp.username, orignalFtp.password)
    try:
        resp = newFtp.storbinary('STOR ' + path + content.filename, content.stream)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except Exception as e:
        return jsonify({'status': 'failed', 'desc': e}), 400
    finally:
        newFtp.quit()

@app.route('/rename', methods=['POST'])
def rename():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    oldPath = data['oldPath']
    newPath = data['newPath']
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.rename(oldPath, newPath)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to rename'}), 400

@app.route('/mkdir', methods=['POST'])
def mkdir():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = data['path']
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.mkd(path)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to make directory'}), 400

@app.route('/rmdir', methods=['POST'])
def rmdir():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = data['path']
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.rmd(path)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to remove directory'}), 400
    
@app.route('/pwd', methods=['POST'])
def pwd():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.pwd()
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to get current directory'}), 400
    
@app.route('/delete', methods=['POST'])
def delete():
    global ftpSessions
    data = request.get_json()
    sessionId = data['sessionId']
    if sessionId not in ftpSessions:
        # session not found
        return jsonify({'status': 'failed', 'desc': 'session not found'}), 400
    
    path = data['path']
    ftp = ftpSessions[sessionId]
    try:
        resp = ftp.delete(path)
        return jsonify({'status': 'success', 'desc': resp}), 200
    except:
        return jsonify({'status': 'failed', 'desc': 'failed to delete'}), 400