var shttp = require('socks5-http-client');
var socks = require('socksv5');
var Client = require('ssh2').Client;
var querystring = require('querystring');


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
  
  var client = new httpClient();
   client.getNodes(function (object){
      console.log("All the nodes of the db: " + JSON.stringify(object));
   });

   client.getNode(44,function (object){
      console.log("For id= 44 I got: " + JSON.stringify(object));
   });

   var jsonObject = {id:"5", pk:"b", idSC:"2"};

   client.setNode(jsonObject,function (object){
      console.log("For the post: " + object);
   });
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
   /*var post_data = querystring.stringify({node:jsonObject});*/
   /*
   var post_data = querystring.stringify({
      'compilation_level' : 'ADVANCED_OPTIMIZATIONS',
      'output_format': 'json',
      'output_info': 'compiled_code',
        'warning_level' : 'QUIET',
        'js_code' : jsonObject
  });*/
   //var post_data = querystring.stringify({node:jsonObject});
   var post_data = querystring.stringify(jsonObject);
   //console.log("Post data: " + post_data);
   var post_req = shttp.get({
        host: 'localhost',
        path: '/api/nodes/',
        method: 'post',
        form: post_data/*,
        headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Content-Length': post_data.length,
        'node': post_data
         }*/
    }, function(response) {
        response.setEncoding('utf8');
        //console.log("Post data: " + post_data);
        response.on('readable', function(){
           var jsonObject = undefined;
           var result = undefined;
           if(!(result = response.read())){
               jsonObject = JSON.parse(result);
           }
           callback(jsonObject);
        });
    });
    //post_req.write(post_data);
    
}
/*
var client = new httpClient();
client.getNodes(function (object){
   console.log("All the nodes of the db: " + JSON.stringify(object));
});

client.getNode(44,function (object){
   console.log("For id= 44 I got: " + JSON.stringify(object));
});

var jsonObject = {node: {id:"5", pk:"b", idSC:"2"}};

client.setNode(jsonObject,function (object){
   console.log("For the post: " + object);
});*/

module.exports = httpClient;