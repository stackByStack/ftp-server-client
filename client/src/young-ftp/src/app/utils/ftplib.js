import LoggerService from './logger'
import { FTP_HOST, FTP_PORT, FTP_ENCODING, FTP_TIMEOUT, FTP_CRLF } from './ftplib/const'
import customSocket from './socket'
import { uuidv4 } from 'uuidv4'
import net from 'net'

class ftpSession {
    constructor(host = FTP_HOST, port = FTP_PORT, user = '', password = '', acct = '', encoding = FTP_ENCODING, timeout = FTP_TIMEOUT) {
        this.host = host;
        this.port = port;
        this.user = user;
        this.password = password;
        this.acct = acct;
        this.encoding = encoding;
        this.timeout = timeout;

        this.logger = new LoggerService('ftpSession', this.sessionId);
        this.sessionId = uuidv4();
        this.socketcmd = new customSocket(this.sessionId, this.logger);
        this.debugLevel = 0;
        this.passiveMode = true;
        this.welcome = '';
        this.lastresp = '';
        this.textEncoder = new TextEncoder(encoding);



        this.logger.info(`ftpSession: host=${host}, port=${port}, user=${user}, password=${password}, acct=${acct}, encoding=${encoding}, timeout=${timeout}`);
    }

    connect(host = '', port = 0, timeout = -999, source_address = null) {
        if (host != '') {
            this.host = host;
        }
        if (port > 0) {
            this.port = port;
        }
        if (timeout != -999) {
            this.timeout = timeout;
        }
        if (this.timeout == 0) {
            this.timeout = FTP_TIMEOUT;
        }
        if (source_address != null) {
            this.source_address = source_address;
        }
        this.logger.info(`connect: host=${this.host}, port=${this.port}, timeout=${this.timeout}, source_address=${this.source_address}`);

        this.socketcmd.connect(this.host, this.port, this.timeout, this.source_address);

        this.welcome = this.getresp();
        return this.welcome;
    }

    async getresp() {
        resp = await this.getmultiline();
        if (this.debugLevel < 1) {
            this.logger.info(`getresp: resp=${resp}`);
        }
        this.lastresp = resp.substring(0, 3);
        const c = resp[0];
        if (c == '1' || c == '2' || c == '3') {
            return resp;
        }
        else {
            this.logger.error(`getresp: resp=${resp}`);
            throw new Error(`getresp: resp=${resp}`);
        }
    }

    voidresp() {
        resp = this.getresp();
        if (resp[0] != '2') {
            this.logger.error(`voidresp: resp=${resp}`);
            throw new Error(`voidresp: resp=${resp}`);
        }
        return resp;
    }

    async getmultiline() {
        line = await this.getline();
        if (line[3] == '-') {
            code = line.substring(0, 3);
            while (true) {
                nextline = await this.getline();
                line += ('\n' + nextline);
                if (nextline.substring(0, 3) == code && nextline[3] != '-') {
                    break;
                }
            }
        }
        return line;
    }

    async getline() {
        line = await this.socketcmd.read();
        if (this.debugLevel < 1)
            this.logger.info(`getline: line=${line}`);
        if (line.endsWith(FTP_CRLF)) {
            line = line.substring(0, -2);
        }
        else if (line.endsWith('\r') || line.endsWith('\n')) {
            line = line.substring(0, -1);
        }

        return line;
    }

    putline(line) {
        if (this.debugLevel < 1)
            this.logger.info(`putline: line=${line}`);
        // if \r or \n in line,
        line += FTP_CRLF;
        //encode line to utf-8
        this.socketcmd.write(this.textEncoder.encode(line));
    }

    putcmd(line) {
        if (this.debugLevel < 1)
            this.logger.info(`putcmd: line=${line}`);
        this.putline(line);
    }

    sendcmd(cmd) {
        this.putcmd(cmd);
        return this.getresp();
    }

    voidcmd(cmd) {
        this.putcmd(cmd);
        return this.voidresp();
    }

    sendport(host, port) {
        const hbytes = host.split('.');
        const pbytes = [String(port >> 8), String(port & 0xff)];
        const bytes = [...hbytes, ...pbytes];
        const cmd = 'PORT ' + bytes.join(',');
        return this.voidcmd(cmd);
    }

    makeport() {
        return new Promise((resolve, reject) => {
            const server = net.createServer({ allowHalfOpen: true });

            server.listen(0, '0.0.0.0', () => {
                const { port } = server.address();
                const host = server.address().address;

                let resp = this.sendport(host, port);
                if (resp[0] !== '2') {
                    this.logger.error(`makeport: resp=${resp}`);
                    reject(new Error(`Failed to send port: ${resp}`));
                    return;
                }

                if (this.timeout !== 0) {
                    server.setTimeout(this.timeout);
                }

                resolve(server);
            });
        });
    }

    makepasv() {
        const resp = this.parse227(this.sendcmd('PASV'));
        if (!resp) {
            this.logger.error(`makepasv: resp=${resp}`);
            return null;
        }
        else {
            return resp;
        }
    }

    parse227(resp) {
        if (resp.slice(0, 3) !== '227') {
            this.logger.error(`parse227: resp=${resp}`);
            throw new Error(`parse227: resp=${resp}`);
        }

        const re = /(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)/;
        const match = resp.match(re);

        if (!match) {
            this.logger.error(`parse227: Invalid response format: ${resp}`);
            throw new Error(`parse227: Invalid response format: ${resp}`);
        }

        const numbers = match.slice(1);
        const host = numbers.slice(0, 4).join('.');
        const port = (parseInt(numbers[4], 10) << 8) + parseInt(numbers[5], 10);
        return [host, port];
    }

    parse150(resp) {
        if (resp.slice(0, 3) !== '150') {
            this.logger.error(`parse150: resp=${resp}`);
            throw new Error(`parse150: resp=${resp}`);
        }

        const re = /150 .* \((\d+) bytes\)/i;
        const match = resp.match(re);

        if (!match) {
            this.logger.error(`parse150: Invalid response format: ${resp}`);
            return null;
        }

        return parseInt(match[1], 10);
    }

    async ntransfercmd(cmd, rest = null) {
        let size = null;
        let conn = null;
        let resp = null;
        if (this.passiveMode) {
            const [host, port] = this.makepasv();
            const conn = new net.Socket();
            conn.setTimeout(this.timeout);

            const connectPromise = new Promise((resolve, reject) => {
                conn.connect(port, host, () => {
                    this.logger.info(`ntransfercmd: connect: host=${host}, port=${port}`);
                    resolve();
                });

                conn.on('error', (error) => {
                    this.logger.error(`ntransfercmd: connect error: ${error}`);
                    reject(error);
                });
            });

            try {
                await connectPromise;

                if (rest !== null) {
                    this.sendcmd(`REST ${rest}`);
                }
                resp = this.sendcmd(cmd);

                if (resp[0] === '2') {
                    resp = this.getresp();
                }
                if (resp[0] !== '1') {
                    this.logger.error(`ntransfercmd: resp=${resp}`);
                    throw new Error(`ntransfercmd: resp=${resp}`);
                }
            } catch (error) {
                this.logger.error(`ntransfercmd: error=${error}`);
                this.closeSocketCmd(conn);
                throw new Error(`ntransfercmd: error=${error}`);
            }
        }
        else {
            sock = await this.makeport();
            if (rest !== null) {
                this.sendcmd(`REST ${rest}`);
            }
            resp = this.sendcmd(cmd);
            if (resp[0] == '2') {
                resp = this.getresp();
            }
            if (resp[0] != '1') {
                this.logger.error(`ntransfercmd: resp=${resp}`);
                throw new Error(`ntransfercmd: resp=${resp}`);
            }
            conn, _ = sock.accept();
            conn.setTimeout(this.timeout);

        }
        if (resp.substring(0, 3) == '150') {
            size = this.parse150(resp);
        }

        return [conn, size];
    }

    async transfercmd(cmd, rest = null) {
        [conn, size] = await this.ntransfercmd(cmd, rest);
        return conn;
    }

    login(user = '', password = '', acct = '') {
        if (user == '') {
            user = 'anonymous';
        }
        if (user == 'anonymous' && (password == '' || password == '-')) {
            password += 'anonymous@';
        }
        resp = this.sendcmd('USER ' + user);
        if (resp[0] == '3') {
            resp = this.sendcmd('PASS ' + password);
        }
        if (resp[0] == '3') {
            resp = this.sendcmd('ACCT ' + acct);
        }
        if (resp[0] != '2') {
            this.logger.error(`login: resp=${resp}`);
            throw new Error(`login: resp=${resp}`);
        }
        return resp;
    }

    retrbinary(cmd, callback, blocksize = 8192, rest = null) {
        this.voidcmd('TYPE I');
        conn = null;
        try {
            conn = this.transfercmd(cmd, rest);
        } catch (error) {
            this.logger.error(`retrbinary: error=${error}`);
            throw new Error(`retrbinary: error=${error}`);
        }
        try {
            while (true) {
                data = conn.read(blocksize);
                if (!data) {
                    break;
                }
                callback(data);
            }
        } catch (error) {
            this.logger.error(`retrbinary: error=${error}`);
            throw new Error(`retrbinary: error=${error}`);
        }
        conn.close();
        return this.voidresp();
    }






    /**
     * @brief: close the socket of cmd and handle possible error or exception and record it in the log
     * @param {socketcmd} socketcmd 
     */
    closeSocketCmd(socketcmd) {
        // const logger = this.logger;
        socketcmd.on('close', function () {
            this.logger.info('Socket closed successfully');
        }.bind(this));

        socketcmd.on('error', function (error) {
            this.logger.error(`Error occurred while closing socket: ${error}`);
            // Log the error in your log file or log management system
            // logError(error);
        }.bind(this));

        socketcmd.destroy();
    }

    destroy() {
        //safely close the socket of cmd and handle possible error or exception and record it in the log
        this.closeSocketCmd(this.socketcmd);
    }

    getWelcome() {
        return this.welcome;
    }

    set_pasv(passive) {
        this.passiveMode = passive;
    }

    set_debuglevel(level) {
        this.debugLevel = level;
    }

}

export default ftpSession;
