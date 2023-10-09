const winston = require('winston');
const moment = require('moment');

//create a custom class of logger that is based on winston
class LoggerService {
    constructor(route) {
        this.log_data = null;
        this.route = route;
        const timestamp = moment().format('YYYY-MM-DD_HH-mm-ss-SSS');
        const filename = `./logs/${this.route}_${timestamp}.log`;

        this.logger = winston.createLogger({
        level: 'info',
        format: winston.format.json(),
        defaultMeta: { service: route },
        transports: [
            new winston.transports.File({
            filename: filename,
            level: 'info',
            }),
        ],
        });
    }
    
    setLogData(log_data) {
        this.log_data = log_data;
    }
    
    async info(message) {
        const data = {
            timestamp: moment().format('YYYY-MM-DD HH:mm:ss.SSS'),
        };
        this.logger.log('info', message, { log_data: data });
    }
    
    async error(message) {
        const data = {
            timestamp: moment().format('YYYY-MM-DD HH:mm:ss.SSS'),
        };
        this.logger.log('error', message, { log_data: data });
    }

    async warn(message) {
        const data = {
            timestamp: moment().format('YYYY-MM-DD HH:mm:ss.SSS'),
        };
        this.logger.log('warn', message, { log_data: data });
    }
}

// Create a logger instance
const logger = new LoggerService('ftp');

export default logger;