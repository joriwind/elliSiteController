var ffi = require('ffi');
var ref = require('ref');
var util            = require('util');
var events          = require('events');

var Dtls = function(arg){
   if (!arg){
      console.log("Specify options for dtls communication");
      arg = {}
   }
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


util.inherits(Dtls, events.EventEmitter)

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
   console.log("NODEJS: Initializing ctx..." + JSON.stringify(arg));
   this.WOLFSSL_CTX = ref.alloc(WOLFSSL_CTXPtr);
   //if(!this.WOLFSSL_CTX)
   //   this.WOLFSSL_CTX = dtls_interface.getTypeWOLFSSL_CTX();
   dtls_interface.initDTLS.async(this.WOLFSSL_CTX, arg.eccCert.toString(), arg.ourCert.toString(), arg.ourKey.toString(), function(err, res){
      if(res < 0 ){
         callback(false);
         this.emit('error','UNKNOWN ERROR');
      }else{
         console.log("DTLS ctx has been initialized ");
         this.emit('initialized',true);
         callback(true);
      }
      return;
   });
}

Dtls.prototype.connectToServer = function(arg, callback){
   console.log("NODEJS: connecting to server...");
   //if(!this.WOLFSSL)
   //   this.WOLFSSL = dtls_interface.getTypeWOLFSSL();
   this.WOLFSSL = ref.alloc(WOLFSSLPtr);
   var that = this;
   dtls_interface.connectToServer.async(this.WOLFSSL, this.WOLFSSL_CTX, arg.host, arg.port, function(err, res){
      if(res <0){
         this.emit('error','ENOTFOUND');
         callback(false);
      }else{
         console.log("Connection established with server ");
         this.emit('connected',true);
         callback(true);
      }
   });
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
   
   dtls_interface.writeDTLS.async(message, function(err, res){
      return;
   });
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

