var socks = require('socksv5');
var Client = require('ssh2').Client;
var http = require('http');

/** Configuration and setup of socks over ssh**/

var ssh_config = {
  host: '192.168.0.159',
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



/** HttpClient classe with all the functions to talk with server over ssh and socks**/
/**
 * Constructor
**/
var HttpClient = function(callback){
   //Sets up the connection with the server
   this.socksServer = new SocksServer(callback);
}

/** Functions for node **/
//Get all the Nodes!
HttpClient.prototype.getNodes = function (callback){
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

//Get a specific Node based upon id of the node
HttpClient.prototype.getNode = function (id, callback){
   return http.get({
        host: 'localhost',
        port: 1080,
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

//Insert an Node into the server's database
HttpClient.prototype.insertNode = function (jsonObject, callback){
   
   var post_data = JSON.stringify({node:jsonObject});
   
   var post_req = http.request({
        host: 'localhost',
        port: 1080,
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

/** Functions for siteController **/
//Insert an Site Controller into the server's database
HttpClient.prototype.insertSiteController = function (jsonObject, callback){
   
   var post_data = JSON.stringify({siteController:jsonObject});
   
   var post_req = http.request({
        host: 'localhost',
        port: 1080,
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


/** Testing purposes **/
var client = new HttpClient(function(){
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

/**Exports**/
module.exports = HttpClient;