const coap  = require('coap'); // or coap 
var DtlsClientAgent = require('./dtlsCoap/dtlsClientAgent');

var server = coap.createServer({type: 'udp6'});

//Contains all the nodes with connections if available
//typedef: {Node, rsinfo, DtlsClientAgent}
var nodes = [];

/* Receive node's 'service announcements'*/
server.on('request', function(req, res){
   console.log("New request: " + JSON.stringify(req));
   if(req.url.split('/')[1] == 'nodes'){
      var node;
      try{
         node = JSON.parse(req.payload);
      }catch(err){
         console.log(err);
         res.code = '5.00';
         return res.end();
      }
      
      switch(node.type){
         case ell-i:
            var nodeAnn = {node: node, rsinfo: req.rsinfo};
            server.emit('new_node', nodeAnn);
            break;
         default:
            //Not supported type
            res.code = '4.01';
            break;
         
      }
   }else{
      res.code = '4.01';
   }
   
   return res.end();
   
      
});

server.listen(function(){});

server.on('new_node', function(nodeAnouncement){
   var nodeConnection;
   /*var object = nodes.filter(function(item){
      return (item.node == node)
   });*/
   
   var rsinfo = nodeAnouncement.rsinfo;
   var node = nodeAnouncement.node;
   
   var nodeConnection = {node: node, rsinfo: rsinfo, connection: undefined};
   
   
   //Initialize Agent, setup ctx --> ready to establish connection.
   console.log("Connection with node: " + node + " on ip: " 
                           + rsinfo.address + " : " + rsinfo.port);
   var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'./certs/server-ecc.pem',
                     'ourCert':'./certs/client-ecc-cert.pem', 'ourKey':'./certs/ecc-client-key.pem', 'host':rsinfo.address, 'port':rsinfo.port});

   dtlsClientAgent.on('connected', function(res){
      console.log("Connected to node: " + JSON.stringify(node));
      
      //Connection established and now ready to fill list
      nodeConnection.connection = dtlsClientAgent;
      nodes.push(nodeConnection);
   });

   //If something happens with connection
   dtlsClientAgent.on('error', function(err){
      console.log('Something went wrong in agent: ' + err);
      //TODO: what error? general error?, certificate error?
      
      //Because connection was cancelled because of error, delete node.
      var nodeConnection = nodes.filter(function(item){
         return (item.connection == dtlsClientAgent)
      });
      if(nodeConnection != undefined){
         var index = nodes.indexOf(nodeConnection);
         if(index > -1){
            nodes.splice(index, 1);
            console.log("Removed this node from list: " + nodeConnection.node);
         }else{
            console.log("Should not be able to get here?");
         }
      }else{
         console.log("Tried to remove non existing object from list?");
      }
      
   });
});

