var http = require('http');

var Client = function(){}

Client.prototype.getNodes = function (callback){
   return http.get({
        host: 'localhost',
        port: 8000,
        path: '/api/nodes'
    }, function(response) {
        // Continuously update stream with data
        var body = '';
        response.on('data', function(d) {
            body += d;
        });
        response.on('end', function() {

            // Data reception is done, do whatever with it!
            var parsed = JSON.parse(body);
            callback(parsed);
        });
    });
}

var httpClient = new Client();
httpClient.getNodes(function (object){
   console.log(object);
});

module.exports = Client;