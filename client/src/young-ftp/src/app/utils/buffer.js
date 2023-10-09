import { Mutex } from 'async-mutex';
// import logger from './logger';

class threadSafeBuffer {
  constructor(sessionId, logger) {
    this.queue = [];
    this.buffer = '';
    this.mutex = new Mutex();
    this.sessionId = sessionId;
    this.logger = logger;
  }

  async push(data) {
    const release = await this.mutex.acquire();
    try {
      this.queue.push(data);
      await this.processBuffer();
    } finally {
      release();
    }
  }

  async processBuffer() {
    while (this.buffer.length > 0) {
      const index = this.buffer.indexOf('\n');
      if (index === -1) {
        break; // No complete line yet, wait for more data
      }

      const line = this.buffer.substring(0, index + 1);
      this.queue.push(line);
      this.buffer = this.buffer.substring(index + 1);
    }
  }

  async pop() {
    while (this.queue.length === 0) {
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    const release = await this.mutex.acquire();
    try {
      this.logger.info(`pop: message=${this.queue[0]}`);
      return this.queue.shift();
    } finally {
      release();
    }
  }
}

export default threadSafeBuffer;