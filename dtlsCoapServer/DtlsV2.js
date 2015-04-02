var ffi = require('ffi');
var ref = require('ref');

var Dtls = function(){
   
}

// typedefs
var CYASSL_CTX = ref.types.void; // we don't know what the layout of "CYASSL_CTX" looks like
var CYASSL = ref.types.void;// we don't know what the layout of "CYASSL" looks like
//var arg = ref.types.cstring;
//var stringPtr = ref.refType('string');
var charPtr = ref.refType(ref.types.char);
var argument = "Hello to JavaScript";


var dtls_interface = ffi.Library('./dtls_interfaceV2', {
  'initDTLS': [ 'pointer', [ ] ],
  'readDTLS': ['void',['pointer', 'pointer']],
  'writeDTLS': ['void',['pointer','string']],
  
});

/**
var readCallback = ffi.Callback('void', [ref.refType(ref.types.char)], 
                            function (buffer) {  
      var message = buffer.readCString(buffer,0);
      console.log("bufRCS: " + message);
      //callback(message);
      this.write(message); //send back
      //dtlsnew.read();
      console.log("callback worked")
});**/



Dtls.prototype.createServer = function(options, callback){
   console.log("NODEJS: CreateServer");
   this.CYASSL = dtls_interface.initDTLS();
   console.log("XXXXXX       XXXXXXXXXX" + this.CYASSL);
   callback(true);
}

Dtls.prototype.read = function(callback){
   dtls_interface.readDTLS(this.CYASSL, ffi.Callback('void', [ref.refType(ref.types.char)], 
                            function (buffer) {  
      var message = buffer.readCString(buffer,0);
      console.log("bufRCS: " + message);
      //callback(message);
      callback(message); //send back
      //dtlsnew.read();
      console.log("callback worked")
   }));
   
}

Dtls.prototype.write = function(message){
   
   dtls_interface.writeDTLS(this.CYASSL,message);
}



Dtls.prototype.send = function(message, number, msglen, port, address, ack){
   this.write(message);
   var err;
   if(typeof ack === 'function'){
      ack(err);
   }
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

var dtls = new Dtls();
dtls.testRepeat();

module.exports = Dtls;

