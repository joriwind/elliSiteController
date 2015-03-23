var shttp = require('socks5-http-client');
var socks = require('socksv5');
var Client = require('ssh2').Client;
/*
var conn = new Client();
conn.on('ready', function(){
   console.log('Client :: ready');
   conn.forwardOut('localhost', 9000, '127.0.0.1',8000, function (err, stream){
      if (err) throw err;
      
      /*stream.on('close', function() {
         console.log('TCP :: CLOSED');
         conn.end();
      }).on('data', function(data) {
         console.log('TCP :: DATA: ' + data);
      }).end([
         'HEAD / HTTP/1.1',
         'User-Agent: curl/7.27.0',
         'Host: 127.0.0.1',
         'Accept: *//*', //delete /
         //'Connection: close',
         '',
         ''
      ].join('\r\n'));
   });
   
}).connect({
   host: '127.0.0.1',
   port: 2222,
   username: 'vagrant',
   privateKey: require('fs').readFileSync('id_rsa')
});*/

var ssh_config = {
  host: '127.0.0.1',
  port: 2222,
   username: 'vagrant',
   privateKey: require('fs').readFileSync('id_rsa')
};

socks.createServer(function(info, accept, deny) {
  // NOTE: you could just use one ssh2 client connection for all forwards, but
  // you could run into server-imposed limits if you have too many forwards open
  // at any given time
  var conn = new Client();
  conn.on('ready', function() {
    conn.forwardOut('localhost',
                    9000,
                    '127.0.0.1',
                    8000,
                    function(err, stream) {
      if (err) {
        conn.end();
        return deny();
      }

      var clientSocket;
      if (clientSocket = accept(true)) {
        stream.pipe(clientSocket).pipe(stream).on('close', function() {
          conn.end();
        });
      } else
        conn.end();
    });
  }).on('error', function(err) {
    deny();
  }).connect(ssh_config);
}).listen(1080, 'localhost', function() {
  console.log('SOCKSv5 proxy server started on port 1080');
}).useAuth(socks.auth.None());

var httpClient = function(){}

httpClient.prototype.getNodes = function (callback){
   return shttp.get({
        host: 'localhost',
        path: '/api/nodes'
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           console.log(res.read());
        });
        /*// Continuously update stream with data
        var body = '';
        response.on('data', function(d) {
            body += d;
        });
        response.on('end', function() {

            // Data reception is done, do whatever with it!
            var parsed = JSON.parse(body);
            callback(parsed);
        });*/
    });
}

var client = new httpClient();
client.getNodes(function (object){
   console.log(object);
});

module.exports = httpClient;