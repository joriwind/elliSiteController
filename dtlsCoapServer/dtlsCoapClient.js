const coap  = require('coap'); // or coap 
var DtlsClientAgent = require('./dtlsClientAgent');

//Initialize Agent, setup ctx --> ready to establish connection.
var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'', 'ourCert':'', 'ourKey':'', 'host':'::1', 'port':5683});

console.log("Print of variable: " + dtlsClientAgent);

   
var req   = coap.request({'hostname':'', 'pathname':'/Lights', 'agent':dtlsClientAgent});
req.on('error', function(err){
   console.log("Could not connect to server");
   req.end()
});
req.on('response', function(res) {
   res.pipe(process.stdout)
   req.end()
})



