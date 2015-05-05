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
  server.on('awaitingConnection', function(){
     //Initialize Agent, setup ctx --> ready to establish connection.
      var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'', 'ourCert':'', 'ourKey':'', 'host':'::1', 'port':5683});

      console.log("Print of variable: " + dtlsClientAgent);
      var agent = dtlsClientAgent;
      dtlsClientAgent.on('connected', function(res){
         console.log("Starting request");
         var req   = coap.request({'port':5683, 'hostname':'', 'pathname':'/Lights', 'agent':agent});
         req.on('error', function(err){
            console.log("Something went wrong in request: " + err);
         });
         req.on('response', function(res) {
            res.pipe(process.stdout)
         })
         req.end()
      });

      dtlsClientAgent.on('error', function(err){
         console.log('Something went wrong in agent: ' + err);
      });
  });
})


