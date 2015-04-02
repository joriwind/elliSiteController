const coap  = require('coap'); // or coap 
var Agent = require('./dtlsAgent');

var agent = new Agent({'type':'dtls4S'});


agent._connect(function(ready){
   //return bool;
   console.log("Connect ready: " + ready);
   
   var req   = coap.request({'url':'coap://localhost/Matteo', 'agent':agent});
   req.on('response', function(res) {
     res.pipe(process.stdout)
   })
   agent._listen();
   req.end()
})
