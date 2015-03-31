var ffi = require('ffi');
var ref = require('ref');

var dtls = function(){
   
}

// typedefs
var CYASSL_CTX = ref.types.void; // we don't know what the layout of "CYASSL_CTX" looks like
var CYASSL = ref.types.void;// we don't know what the layout of "CYASSL" looks like
//var arg = ref.types.cstring;
//var stringPtr = ref.refType('string');
var charPtr = ref.refType(ref.types.char);
var argument = "Hello to JavaScript";


var dtls_interface = ffi.Library('./dtls_interface', {
  'init': [ 'pointer', [ ] ],
  'AwaitDGram': ['pointer',['pointer']],
  'listenDTLS': ['int',['pointer', ref.refType(ref.types.char)]],
  'writeDTLS': ['int',['pointer','string']],
  'closeDTLS': ['void',['pointer']]
});


dtls.prototype.createServer = function(options, callback){
   /*dtls_interface.init.async(function(err,res){
      if(err) throw err;
      this.CYASSL_CTX = res;
      callback(true);
   });*/
   this.CYASSL_CTX = dtls_interface.init();
   callback(true);
}

dtls.prototype.listenForNode = function(callback){
   /*dtls_interface.AwaitDGram.async(this.CYASSL_CTX, function(err, res){
      if(err) throw err;
      this.CYASSL = res;
      callback(true);
   });*/
   this.CYASSL = dtls_interface.AwaitDGram(this.CYASSL_CTX);
   callback(true);
}

dtls.prototype.read = function(callback){
   var buffer = new Buffer(4096);
   buffer.type = ref.types.char;
   if(dtls_interface.listenDTLS(this.CYASSL, buffer) == 0){
      var message = buffer.readCString(buffer,0);
      console.log("bufRCS: " + message);
      callback(message);
   }
}

dtls.prototype.write = function(message){
   dtls_interface.writeDTLS(this.CYASSL,message);
}

dtls.prototype.close = function(){
   dtls_interface.closeDTLS(CYASSL);
}

dtls.prototype.send = function(message, number, msglen, port, address, ack){
   write(message);
   var err;
   if(typeof ack === 'function'){
      ack(err);
   }
}

dtls.prototype.testRepeat = function(){
   var dtlsnew = new dtls();
   dtlsnew.createServer("", function(bool){
      if(bool){
         dtlsnew.listenForNode(function(boolListen){
            
            dtlsnew.read(function(message){
               dtlsnew.write(message);
            });
            
         });
      }
   });
}

