const coap  = require('coap'); // or coap 
var DtlsClientAgent = require('./dtlsCoap/dtlsClientAgent');
var util = require('util');

/* Set up server, listening to service announcements */
var server = coap.createServer({type: 'udp6'});

/* Set up terminal, listening for commands */
process.stdin.resume();
process.stdin.setEncoding('utf8');

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
         case 'ell-i':
            var nodeAnn = {node: node, rsinfo: req.rsinfo};
            server.emit('new_node', nodeAnn);
            break;
         default:
            //Not supported type
            console.log("Not supported type");
            res.code = '4.01';
            break;
         
      }
   }else{
      console.log("Not supported route");
      res.code = '4.01';
   }
   
   return res.end();
   
      
});

server.listen(function(){
   console.log("Started listening for nodes");
});

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
                           + rsinfo.address + " : " + 11111); //Juiste poort?
   var dtlsClientAgent = new DtlsClientAgent({'type':'dtls_client', 'eccCert':'./certs/server-ecc.pem',
                     'ourCert':'./certs/client-ecc-cert.pem', 'ourKey':'./certs/ecc-client-key.pem', 'host':rsinfo.address, 'port':11111});

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

/** Terminal usage **/
process.stdin.on('data', function (text) {
   //console.log('received data:', util.inspect(text));
   var command = (text.split('\n'))[0].split(' '); //remove '\n' and then split on spaces
   console.log("Command is: " + command[0] + " ,full with attributes: " + command);
   switch(command[0]){
      case 'list':
         //show list
         console.log("List of nodes: " + JSON.stringify(nodes));
         break;
      case 'indexof':
         if(command[1]){
            nodes.forEach(function(object){
               if(object.node.name == command[1]){
                  var index = nodes.indexOf(object);
               }
            });
         }else{
            console.log("Give name of node");
         }
         break;
      case 'ping':
         if(command[1]){
            nodes.forEach(function(object){
               if(object.node.name == command[1]){
                  executeRequest(object.connection, '/ping', undefined, function(payload){
                     console.log("Response of server: " + payload.toString());
                  });
                  return;
               }else{
                  if(nodes.indexOf(object) == (nodes.length - 1)){
                     console.log("No node available under that name");
                  }
               }
            });
            
         }else{
            console.log("Give name of node to ping");
         }
         break;
      case 'quit':
         done();
         break;
      default:
         console.log("Did not understand command: " + command[0]);
         break;
   }
   
});

function executeRequest(dtlsClientAgent, route, payload, callback){
   var req   = coap.request({pathname: route, 'agent':dtlsClientAgent});
   
   if(payload){
      req.write(JSON.stringify(payload));
   }
   
   req.on('error', function(err){
      console.log("Something went wrong in request: " + err);
   });
   req.on('response', function(res) {
      //console.log("Response of server: " + JSON.stringify(res.payload));
      callback(res.payload);
      //res.pipe(process.stdout)
   })
   req.end()
}

function done() {
   console.log('Now that process.stdin is paused, there is nothing more to do.');
   
   process.exit();
}

