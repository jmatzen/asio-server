const http2 = require('http2');

const server = http2.createServer();

server.on('stream', (stream, headers) => {
  stream.respond({
    'content-type': 'text/html',
    ':status': 200
  });
  stream.end('<h1>Hello, h2c!</h1>');
});

server.listen(8080);