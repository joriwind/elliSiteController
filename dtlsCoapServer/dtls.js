var ffi = require('ffi');
var ref = require('ref');

var Dtls = function(arg){
   if (!arg)
      console.log("Specify options for dtls communication");
      arg = {}
   if(arg.ctx){
      WOLFSSL_CTX = ctx;
   }else{
      console.log(JSON.stringify(arg));
      this.initDTLS(arg, function(bool){
         if(bool == false){
            console.log("Error occurred in DTLS initialization");
         }
      });
   }
   
}

// typedefs
var WOLFSSL_CTX = ref.types.void; // we don't know what the layout of "WOLFSSL_CTX" looks like
var WOLFSSL_CTXPtr = ref.refType(WOLFSSL_CTX);
var WOLFSSL = ref.types.void;// we don't know what the layout of "WOLFSSL" looks like
var WOLFSSLPtr = ref.refType(WOLFSSL);

var argument = "Hello to JavaScript";

var dtls_interface = ffi.Library('./dtls_interface_ipv6', {
  'initDTLS': [ 'int', [WOLFSSL_CTXPtr, 'string','string','string' ] ],
  'connectToServer': ['int', [WOLFSSLPtr, WOLFSSL_CTXPtr, 'string', 'int']],
  'readDTLS': ['void',[WOLFSSLPtr, 'pointer']],
  'writeDTLS': ['void',[WOLFSSLPtr,'string']],
  'closeDTLS': ['void',[]],
  'getTypeWOLFSSL_CTX': [WOLFSSL_CTXPtr, []],
  'getTypeWOLFSSL': [WOLFSSLPtr, []]
});

Dtls.prototype.initDTLS = function(arg, callback){
   arg.eccCert = "";
   arg.ourCert = "";
   arg.ourKey = "";
   console.log("NODEJS: Initializing ctx..." + JSON.stringify(arg));
   this.WOLFSSL_CTX = ref.alloc(WOLFSSL_CTXPtr);
   //if(!this.WOLFSSL_CTX)
   //   this.WOLFSSL_CTX = dtls_interface.getTypeWOLFSSL_CTX();
   if(dtls_interface.initDTLS(this.WOLFSSL_CTX, arg.eccCert.toString(), arg.ourCert.toString(), arg.ourKey.toString()) <0){
   //if(dtls_interface.initDTLS(this.WOLFSSL_CTX, eccCert, ourCert, ourKey) <0){
      callback(false);
   }
   console.log("DTLS ctx has been initialized ");
   callback(true);
}

Dtls.prototype.connectToServer = function(arg, callback){
   console.log("NODEJS: connecting to server...");
   //if(!this.WOLFSSL)
   //   this.WOLFSSL = dtls_interface.getTypeWOLFSSL();
   this.WOLFSSL = ref.alloc(WOLFSSLPtr);
   if(dtls_interface.connectToServer(this.WOLFSSL, this.WOLFSSL_CTX, arg.host, arg.port) <0){
      callback(false);
   }
   console.log("Connection established with server ");
   callback(true);
}

Dtls.prototype.recvfrom = function(callback){
   dtls_interface.readDTLS.async(this.WOLFSSL, ffi.Callback('void', [ref.refType(ref.types.char)], 
                            function (buffer) {  
      var message = buffer.readCString(buffer,0);
      console.log("bufRCS: " + message);
      //callback(message);
      var buff = new Buffer(message);
      callback(buff); //send back buffer
      //dtlsnew.read();
      console.log("callback worked")
   }), function(err, res){
      return;
   });
   
}

Dtls.prototype.sendto = function(message){
   
   dtls_interface.writeDTLS(message);
}



Dtls.prototype.send = function(message, number, msglen, port, address, ack){
   console.log("Send message\n");
   this.sendto(message.toString()); //message is of type: Buffer
   var err = 0;
   if(typeof ack === 'function'){
      ack(err);
   }
}

Dtls.prototype.send = function(message, number, msglen, port, address){
   console.log("Send message\n");
   this.write(message.toString()); //message is of type: Buffer
   
}

Dtls.prototype.address = function(){
   //TODO: make this work
   return 5;
}

Dtls.prototype.close = function(){
   
}

Dtls.prototype.testRepeat = function(){
   var dtlsnew = new Dtls();
   dtlsnew.createServer("", function(bool){
      if(bool){
         console.log("NODEJS: Start read");
         dtlsnew.read(function(message){
            dtlsnew.write(message);
         });
      }
   });
}

//var dtls = new Dtls();
//dtls.testRepeat();

module.exports = Dtls;

