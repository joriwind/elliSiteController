var socks = require('socksv5');
var Client = require('ssh2').Client;
var http = require('http');
var querystring = require('querystring');


var ssh_config = {
  host: '127.0.0.1',
  port: 2222,
   username: 'vagrant',
   privateKey: require('fs').readFileSync('id_rsa')
};

var socksConfig = {
  proxyHost: 'localhost',
  proxyPort: 1080,
  auths: [ socks.auth.None() ]
};

var SocksServer = function (callback){
   return socks.createServer(function(info, accept, deny) {
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
     callback();
   }).useAuth(socks.auth.None());
}




var httpClient = function(callback){
   this.socksServer = new SocksServer(callback);
}

httpClient.prototype.getNodes = function (callback){
   return http.get({
        host: 'localhost',
        port: 1080,
        path: '/api/nodes/',
        agent: new socks.HttpAgent(socksConfig)
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           var jsonObject = JSON.parse(response.read());
           callback(jsonObject);
        });
    });
}

httpClient.prototype.getNode = function (id, callback){
   return http.get({
        host: 'localhost',
        port: '1080',
        path: '/api/nodes/' + id,
        agent: new socks.HttpAgent(socksConfig)
    }, function(response) {
        response.setEncoding('utf8');
        response.on('readable', function(){
           var jsonObject = JSON.parse(response.read());
           callback(jsonObject);
        });
    });
}

httpClient.prototype.insertNode = function (jsonObject, callback){
   
   var post_data = JSON.stringify({node:jsonObject});
   
   var post_req = http.request({
        host: 'localhost',
        port: '1080',
        path: '/api/nodes/',
        method: 'post',
        form: post_data,
        agent: new socks.HttpAgent(socksConfig),
        headers: {
        'Content-Type': 'application/json',
        'Content-Length': post_data.length,
        'node': post_data
         }
    }, function(response) {
        response.setEncoding('utf8');
        
        // Continuously update stream with data
        var body = '';
        response.on('data', function(d) {
            body += d;
        });
        response.on('end', function() {
           var jsonObject;
           if(body){
               jsonObject = JSON.parse(body);
           }
           callback(jsonObject);
        });
    });
    
    post_req.write(post_data);
    post_req.end();
    
}

httpClient.prototype.insertSiteController = function (jsonObject, callback){
   
   var post_data = JSON.stringify({siteController:jsonObject});
   
   var post_req = http.request({
        host: 'localhost',
        port: '1080',
        path: '/api/sitecontrollers/',
        method: 'post',
        form: post_data,
        agent: new socks.HttpAgent(socksConfig),
        headers: {
        'Content-Type': 'application/json',
        'Content-Length': post_data.length,
        'node': post_data
         }
    }, function(response) {
        response.setEncoding('utf8');
        //console.log("Post data: " + post_data);
        response.on('readable', function(){
           var jsonObject;
           var result;
           if(!(result = response.read())){
               jsonObject = JSON.parse(result);
           }
           callback(jsonObject);
        });
    });
    
    post_req.write(post_data);
    post_req.end();
    
}

var client = new httpClient(function(){
   /*
   client.getNodes(function (object){
      console.log("All the nodes of the db: " + JSON.stringify(object));
   });

   client.getNode(44,function (object){
      console.log("For id= 44 I got: " + JSON.stringify(object));
   });
   */
   
   var node = {id:"6", pk:"C", "idSC":"5"};
   client.insertNode(node,function (object){
      console.log("Response of POST node: " + JSON.stringify(object) );
   });
   
   /*
   var siteController = {id:"6"};
   
   client.insertSiteController(siteController,function (object){
      console.log("Response of POST siteController: " + JSON.stringify(object) );
   });*/
});


module.exports = httpClient;