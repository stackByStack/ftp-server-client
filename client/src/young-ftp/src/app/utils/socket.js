//a custom socket with thread-safe buffer
import logger from "./logger";
import threadSafeBuffer from "./buffer";
import { Socket } from 'net'
class customSocket {
    constructor(sessionId, logger) {
        this.socket = new Socket();
        this.sessionId = sessionId;
        this.logger = logger;
        this.buffer = new threadSafeBuffer(sessionId, logger);

        this.socket.on('data', async function (data) {
            this.logger.info(`customSocket: data=${data}`);
            await this.buffer.push(data);
        }.bind(this));
        this.socket.on('close', function () {
            this.logger.info('customSocket: Socket closed successfully');
        }.bind(this));
        this.socket.on('error', function (error) {
            this.logger.error(`customSocket: Error occurred while closing socket: ${error}`);
        }.bind(this));
    }

    getSocket() {
        return this.socket;
    }

    /**
     * @brief: connect to the server with the given host, port, timeout and source_address
     * @param {*} host 
     * @param {*} port 
     * @param {*} timeout 
     * @param {*} source_address 
     */
    connect(host = '', port = 0, timeout = -999, source_address = null) {
        this.socket.setTimeout(timeout);
        this.socket.connect(port, host, function () {
            this.logger.info(`customSocket: connect: host=${host}, port=${port}, timeout=${timeout}, source_address=${source_address}`);
        }.bind(this));
    }

    /**
     * @brief read data from the buffer and return a single line
     * @returns {Promise} a promise that resolves to the data read from the buffer
     */
    async read() {
        const data = await this.buffer.pop();
        this.logger.info(`customSocket: read: data=${data}`);
        return data;
    }

    /**
     * @brief: write data to the socket
     * @param {*} data 
     */
    write(data) {
        this.logger.info(`customSocket: write: data=${data}`);
        sendAll(client, data)
            .then((totalSent) => {
                this.logger.info(`customSocket: write: totalSent=${totalSent}`);
            })
            .catch((error) => {
                this.logger.error(`customSocket: write: error=${error}`);
            });
    }

    /**
     * @brief: close the socket
     */
    close() {
        this.socket.end();
    }

    sendAll(data) {
        return new Promise((resolve, reject) => {
            let totalSent = 0;

            function sendChunk() {
                const chunk = data.slice(totalSent);
                this.socket.write(chunk, (error) => {
                    if (error) {
                        reject(error);
                        return;
                    }

                    totalSent += chunk.length;

                    if (totalSent === data.length) {
                        resolve(totalSent);
                    } else {
                        sendChunk();
                    }
                });
            }

            sendChunk();
        });
    }
}

export default customSocket;