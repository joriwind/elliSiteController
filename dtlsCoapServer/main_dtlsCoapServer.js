var coap        = require('coap');
var DtlsClientAgent = require('./dtlsClientAgent');
var Server  = require('./dtlsServer');
var server      = new Server({ type: 'udp6' })

server.on('request', function(req, res) {
  res.end('Hello ' + req.url.split('/')[1] + '\n')
})

server.on('error', function(err){
   console.log("Error in server: "+ err);
});

// the default CoAP port is 5683
server.listen(function() {
  
});

server.on('awaitingConnection', function(){
     console.log("Awainting an connection");
     //Initialize Agent, setup ctx --> ready to establish connection.
      
  });


