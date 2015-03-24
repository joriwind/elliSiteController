var shttp = require('socks5-http-client');
var socks = require('socksv5');
var Client = require('ssh2').Client;
var querystring = require('querystring');

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
        path: '/api/nodes/'
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           var jsonObject = JSON.parse(response.read());
           callback(jsonObject);
        });
    });
}

httpClient.prototype.getNode = function (id, callback){
   return shttp.get({
        host: 'localhost',
        path: '/api/nodes/' + id
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           var jsonObject = JSON.parse(response.read());
           callback(jsonObject);
        });
    });
}

httpClient.prototype.setNode = function (jsonObject, callback){
   var post_data = querystring.stringify({'node':jsonObject});
   
   var post_req = shttp.get({
        host: 'localhost',
        path: '/api/nodes/',
        method: 'post',
        form: post_data
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           var jsonObject = JSON.parse(response.read());
           callback(jsonObject);
        });
    });
    //post_req.write(post_data);
    
}

var client = new httpClient();
client.getNodes(function (object){
   console.log("All the nodes of the db: " + object);
});

client.getNode(44,function (object){
   console.log("For id= 44 I got: " + object);
});

var jsonObject = {id:"5", pk:"b", idSC:"2"}

client.setNode(JSON.stringify(jsonObject),function (object){
   console.log("For the post: " + object);
});

module.exports = httpClient;