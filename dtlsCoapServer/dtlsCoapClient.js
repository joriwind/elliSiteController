const coap  = require('coap'); // or coap 
var DtlsClientAgent = require('./dtlsClientAgent');

var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'', 'ourCert':'', 'ourKey':'', 'host':'::1', 'port':5683});


   
var req   = coap.request({'hostname':'', 'pathname':'/Lights', 'agent':dtlsClientAgent});
req.on('response', function(res) {
   res.pipe(process.stdout)
})
req.end()

