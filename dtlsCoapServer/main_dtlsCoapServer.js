var coap        = require('coap');
var Server  = require('./dtlsServer');
var server      = new Server({ type: 'udp6' })

server.on('request', function(req, res) {
  console.log("Received message from client: " + req.url.split('/')[1]  + '\nMessage payload:\n' + req.payload+'\n');
  var js = req.payload.title;
  
  
  //console.log("Title: " + js.title);
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


