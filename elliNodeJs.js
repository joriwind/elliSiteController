var coap        = require('coap');
var Server  = require('./dtlsCoap/dtlsServer');
var server      = new Server({ type: 'udp6' , eccCert: './certs/ca-cert.pem',
                                 ourCert: './certs/server-cert.pem', ourKey:'./certs/server-key.der'})

server.on('request', function(req, res) {
   var route = req.url.split('/')[1];
   console.log("Received message from client: " + route  + '\nMessage payload:S:' + req.payload+':E\n');
  
   if(req.payload[0]){
      //Parse packet
      try{
         var js = JSON.parse(req.payload);
         console.log("Packet parsed: " + JSON.stringify(js));
      }catch (e){
        //Could not parse JSON -> send error to client
        console.log(e);
        res.code = '4.06';
        return res.end();
      }
   }
   
   //Endpoints
   switch(route){
      case 'ping':
         console.log("Ping route");
         return res.end('Hello client\n');
         break;
      default:
         console.log("Default, route: " + route);
         res.code = '4.01';
         break;
     
   }
  
   res.end();
  
})

server.on('error', function(err){
   console.log("Error in server: "+ err);
});

// the default CoAP port is 5683
server.listen(11111, function() {
   
});

server.on('awaitingConnection', function(){
   console.log("Awainting an connection");
   //Initialize Agent, setup ctx --> ready to establish connection.
   console.log("Sending an service announcement");
   var req = coap.request('coap://[::1]/nodes');
   var payload = {name: 'node1', type: 'ell-i'}
   
   req.write(JSON.stringify(payload));
   
   req.on('error', function(err){
      console.log("Somehting went wrong in service announcement: " + err);
   });
   
   req.on('response', function(res){
      console.log("Response of something: " + JSON.stringify(res));
      
   });
   
   req.end();
});