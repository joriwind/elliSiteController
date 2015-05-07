const coap  = require('coap'); // or coap 
var DtlsClientAgent = require('./dtlsClientAgent');

//Initialize Agent, setup ctx --> ready to establish connection.
var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'', 'ourCert':'', 'ourKey':'', 'host':'::1', 'port':5683});

console.log("Print of variable: " + dtlsClientAgent);
var agent = dtlsClientAgent;
dtlsClientAgent.on('connected', function(res){
   console.log("Starting request");
   var req   = coap.request({port:5683, hostname:'::1',method: 'GET',  'pathname':'/Lights', agent:agent});
   req.on('error', function(err){
      console.log("Something went wrong in request: " + err);
   });
   req.on('response', function(res) {
      console.log("Response of server: " + res);
      //res.pipe(process.stdout)
   })
   req.end()
});

dtlsClientAgent.on('error', function(err){
   console.log('Something went wrong in agent: ' + err);
});

   




