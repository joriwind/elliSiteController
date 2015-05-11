var ffi = require('ffi');
var ref = require('ref');
var util            = require('util');
var events          = require('events');
var ArrayType = require('ref-array');

var Dtls = function(arg, callback){
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
var MessagePtr = ref.refType(ref.types.char);
var CharArray = ArrayType('char');
var CharArrayPtr = ref.refType(CharArray);
var BUFFF = ref.refType(ref.types.int);


var argument = "Hello to JavaScript";

var dtls_interface = ffi.Library('./dtls_interface_ipv6', {
  'initDTLS': [ 'int', [WOLFSSL_CTXPtr, 'string','string','string', 'int' ] ],
  'connectToServer': ['int', [WOLFSSLPtr, WOLFSSL_CTXPtr, 'string', 'int']],
  'awaitConnection': ['int',[WOLFSSLPtr, WOLFSSL_CTXPtr, 'int', 'pointer', 'pointer']],
  'readDTLS': ['void',[WOLFSSLPtr, 'pointer']],
  'writeDTLS': ['void',[WOLFSSLPtr, BUFFF, 'int']],
  'closeDTLS': ['void',[]],
  'getTypeWOLFSSL_CTX': [WOLFSSL_CTXPtr, []],
  'getTypeWOLFSSL': [WOLFSSLPtr, []]
});

Dtls.prototype.initDTLS = function(arg, callback){
   console.log("NODEJS: Initializing ctx..." + JSON.stringify(arg));
   this.WOLFSSL_CTX = ref.alloc(WOLFSSL_CTXPtr);
   var that = this;
   if(!arg.isServer){
      arg.isServer = 0;
   }
   
   dtls_interface.initDTLS.async(this.WOLFSSL_CTX, arg.eccCert.toString(), arg.ourCert.toString(), arg.ourKey.toString(), arg.isServer, function(err, res){
      if(res < 0 ){
         that.emit('error',res);
         callback(false);
      }else{
         console.log("DTLS ctx has been initialized ");
         that.emit('initialized',true);
         callback(true);
      }
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
         that.emit('error',res);
         callback(false);
      }else{
         console.log("Connection established with server ");
         that.emit('connected',true);
         callback(true);
      }
   });
}

Dtls.prototype.awaitConnection = function(arg, callback){
   console.log("NODEJS: setting up wait...");
   this.client_addrPtr = ref.alloc(ref.refType(ref.refType(ref.types.char)));
   this.client_portPtr = ref.alloc(ref.refType(ref.types.int));
   this.WOLFSSL = ref.alloc(WOLFSSLPtr);
   var that = this;
   dtls_interface.awaitConnection.async(this.WOLFSSL, this.WOLFSSL_CTX, arg.port, this.client_addrPtr, this.client_portPtr, function(err, res){
      if(res <0){
         that.emit('error',res);
         callback(false);
      }else{
         that.client_addr = that.client_addrPtr.deref().readCString(that.client_addrPtr.deref(),0);
         that.client_portPtr.type = ref.types.int
         that.client_port = that.client_portPtr.deref();
         console.log("Connection established with client: " + that.client_addr + " : " + that.client_port);
         that.emit('connected',true);
         callback(true);
      }
   });
}

Dtls.prototype.recvfrom = function(callback){
   console.log("Starting recvfrom thread");
   //var buf = ref.alloc(ref.refType(ref.refType(ref.types.char)));
   /*var buf = new Buffer(100);
   buf.type = ArrayType('char');*/
   var that = this;
   dtls_interface.readDTLS.async(this.WOLFSSL, ffi.Callback('void', [BUFFF, ref.types.int], 
                            function (buf, rcvlen) {  
      //var message = buffer.readCString(buffer,0);
      //var buff = new Buffer(message);
      //buffer.type = ref.refType(ref.types.char);
      //buf.length = rcvlen;
      //console.log("Buffer recv: " + buf + ", Length of buff: " + buf.length);
      var data = ref.reinterpret(buf,  rcvlen, 0);
      //ref.types.int.size *
       /*for (var i = 0; i < rcvlen; i++) {
           ref.types.int.get(data, i * ref.types.int.size);
       }*/
      console.log("Buffer recv: " + data);
      //var string = buffer.deref();
      //var buff = new Buffer(buffer);
      //console.log("Buffer after recv: " + buff);
      //var buff = new Buffer(buffer);
      var rsinfo = {'address':that.client_addr, 'port':that.client_port};
      callback(data, rsinfo); //send back buffer
      //dtlsnew.read();
   }), function(err, res){
      return;
   });
   
}

Dtls.prototype.sendto = function(message, msglen){
   dtls_interface.writeDTLS.async(this.WOLFSSL, message, msglen, function(err, res){
      return;
   });
}



Dtls.prototype.send = function(message, number, msglen, port, address, ack){
   console.log("Send message");
   //this.sendto(message.toString('utf8', 0, msglen)); //message is of type: Buffer
   this.sendto(message, msglen);
   var err = 0;
   if(typeof ack === 'function'){
      ack(err);
   }
}

Dtls.prototype.send = function(message, number, msglen, port, address){
   //console.log("Is ascii encoding: " + message.isEncoding('ascii'));
   //console.log("Is utf8 encoding: " + message.isEncoding('utf8'));
   console.log("Send message");
   //this.sendto(message.toString('utf8', 0, msglen)); //message is of type: Buffer
   this.sendto(message, msglen);
}

Dtls.prototype.address = function(){
   
   if(!this.client_addr){
      return {port: 5683, family: 'IPv6', address: "::1"};
   }
   return {port: this.client_port, family: 'IPv6', address: this.client_addr};
}

Dtls.prototype.close = function(){
   dtls_interface.closeDTLS.async(function(err, res){
      console.log("Connection closed");
   });
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

