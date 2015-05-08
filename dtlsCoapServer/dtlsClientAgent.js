/*
 * Copyright (c) 2013-2015 node-coap contributors.
 *
 * node-coap is licensed under an MIT +no-false-attribs license.
 * All rights not explicitly granted in the MIT license are reserved.
 * See the included LICENSE file for more details.
 */

var bl              = require('bl')
  , Dtls            = require('./Dtls') //interface
  , util            = require('util')
  , events          = require('events')
  , parse           = require('coap-packet').parse
  , generate        = require('coap-packet').generate
  , URL             = require('url')
  , IncomingMessage = require('./node_modules/coap/lib/incoming_message')
  , OutgoingMessage = require('./node_modules/coap/lib/outgoing_message')
  , ObserveStream   = require('./node_modules/coap/lib/observe_read_stream')
  , parameters      = require('./node_modules/coap/lib/parameters')
  , optionsConv     = require('./node_modules/coap/lib/option_converter')
  , RetrySend       = require('./node_modules/coap/lib/retry_send')
  , parseBlock2     = require('./node_modules/coap/lib/helpers').parseBlock2
  , createBlock2    = require('./node_modules/coap/lib/helpers').createBlock2
  , getOption       = require('./node_modules/coap/lib/helpers').getOption
  , maxToken        = Math.pow(2, 32) //First it was 32 but coap-packet max length = 8 --> 2^26
  , maxMessageId    = Math.pow(2, 16)

function Agent(opts) {
   if (!(this instanceof Agent))
    return new Agent()

  if (!opts)
    opts = {}

  if (!opts.type)
    opts.type = 'udp4'

  
   
   if(!opts.eccCert)
      opts.eccCert = ''
   if(!opts.ourCert)
      opts.ourCert = ''
   if(!opts.ourKey)
      opts.ourKey = ''
   if(!opts.host)
      opts.host = '::1'
   if(!opts.port)
      opts.port = 5683

   this._opts = opts

  this._init()
}

util.inherits(Agent, events.EventEmitter)

Agent.prototype._init = function initSock() {
  if (this._sock)
    return

  var that = this
  
  this._sock = new Dtls(this._opts);
  
   
   //When DTLS initialize try to connect with peer
   this._sock.on('initialized', function(bool){
      console.log("DTLS is initialized");
      that._sock.connectToServer(that._opts,function(initReady){
         console.log("DTLS connnectToServer return");
         if(initReady == false){
            console.log("FAIL in connectToServer");
            that.emit('error','ENOTFOUND');
         }
      });
      
   });
   
   //When connected setup recv thread handler
   this._sock.on('connected', function(bool){
      that._sock.recvfrom(function(msg, rsinfo){
         var packet
            , message
            , outSocket

          try {
            packet = parse(msg)
          } catch(err) {
            message = generate({ code: '5.00', payload: new Buffer('Unable to parse packet') })
            that._sock.send(message, 0, message.length,
                            rsinfo.port, rsinfo.address)
            return
          }

          outSocket = that._sock.address();
          that._handle(msg, rsinfo, outSocket)
      });
      
      
      console.log("dtlsAgent: Init complete");
      that.emit('connected', true);
   });
  
  
  //Catch errors of DTLS
   this._sock.on('error', function(err){
      console.log("Error in dtls: " + err);
      that.emit('error', err);
   });
   
  this._msgIdToReq = {}
  this._tkToReq = {}

  this._lastToken = Math.floor(Math.random() * (maxToken - 1))
  this._lastMessageId = Math.floor(Math.random() * (maxMessageId - 1))

  this._closing = false
  this._msgInFlight = 0
  this._requests = 0
}

Agent.prototype._cleanUp = function cleanUp() {
  if (--this._requests !== 0)
    return

  this._closing = true

  if (this._msgInFlight !== 0)
    return

  this._doClose()
}

Agent.prototype._doClose = function() {
  for (var k in this._msgIdToReq)
    this._msgIdToReq[k].sender.reset()

  this._sock.close()
  this._sock = null
}

Agent.prototype._handle = function handle(msg, rsinfo, outSocket) {
  var packet = parse(msg)
    , buf
    , response
    , that = this
    , req = this._msgIdToReq[packet.messageId]
    , ackSent = function(err) {
        if (err && req)
          req.emit('error', err)

        that._msgInFlight--
        if (that._closing && that._msgInFlight === 0) {
          that._doClose()
        }
      }

  if (packet.confirmable) {
    buf = generate({
        code: '0.00'
      , ack: true
      , messageId: packet.messageId
    })

    this._msgInFlight++
    this._sock.send(buf, 0, buf.length, rsinfo.port, rsinfo.address, ackSent)
  }

  if (!req) {
    if (packet.token.length == 4) {
      req = this._tkToReq[packet.token.readUInt32BE(0)]
    }

    if (packet.ack && !req) {
      // nothing to do, somehow there was
      // a duplicate ack
      return
    }

    if (!req) {
      buf = generate({
          code: '0.00'
        , reset: true
        , messageId: packet.messageId
      })

      this._msgInFlight++
      this._sock.send(buf, 0, buf.length, rsinfo.port, rsinfo.address, ackSent)
      return
    }
  }

  if (!packet.confirmable)
    delete this._msgIdToReq[packet.messageId]

  req.sender.reset()

  if (packet.code == '0.00')
    return

  var block2Buff = getOption(packet.options, 'Block2')
  var block2
  // if we got blockwise (2) response
  if (block2Buff) {
    block2 = parseBlock2(block2Buff)
    // check for error
    if (!block2) {
      req.sender.reset()
      return req.emit('error', err)
    }
  }
  if (block2) {
    // accumulate payload
    req._totalPayload = Buffer.concat([req._totalPayload, packet.payload])

    if (block2.moreBlock2) {
      // increase message id for next request
      delete this._msgIdToReq[req._packet.messageId]
      req._packet.messageId = that._nextMessageId()
      this._msgIdToReq[req._packet.messageId] = req 

      // next block2 request
      var block2Val = createBlock2({
        moreBlock2: false,
        num: block2.num+1,
        size: block2.size
      })
      if (!block2Val) {
        req.sender.reset()
        return req.emit('error', err)
      }
      req.setOption('Block2', block2Val)
      req.sender.send(generate(req._packet))

      return
    }
    else {
      // get full payload
      packet.payload = req._totalPayload
      // clear the payload incase of block2
      req._totalPayload = new Buffer(0)
    }
  }

  if (req.response) {
    // it is an observe request
    // and we are already streaming
    return req.response.append(packet)
  }
  else if (block2) {
    delete that._tkToReq[req._packet.token.readUInt32BE(0)]
  }
  else if (!req.url.observe)
    // it is not, so delete the token
    delete that._tkToReq[packet.token.readUInt32BE(0)]

  if (req.url.observe && packet.code !== '4.04') {
    response = new ObserveStream(packet, rsinfo, outSocket)
    response.on('close', function() {
      delete that._tkToReq[packet.token.readUInt32BE(0)]
      that._cleanUp()
    })
  } else {
    response = new IncomingMessage(packet, rsinfo, outSocket)
  }

  req.response = response
  req.emit('response', response)
}

Agent.prototype._nextToken = function nextToken() {
  var buf = new Buffer(4)

  if (++this._lastToken === maxToken)
    this._lastToken = 0

  buf.writeUInt32BE(this._lastToken, 0)

  return buf;
}

Agent.prototype._nextMessageId = function nextToken() {
  if (++this._lastMessageId === maxMessageId)
    this._lastMessageId = 1

  return this._lastMessageId
}

Agent.prototype.request = function request(url) {
  this._init()

  var req
    , response
    , options = url.options || url.headers
    , option
    , that = this

  req = new OutgoingMessage({}, function(req, packet) {
    var buf

    if (url.confirmable !== false) {
      packet.confirmable = true
    }

    if (!(packet.ack || packet.reset)) {
      packet.messageId = that._nextMessageId()
      packet.token = that._nextToken()
    }

    try {
      buf = generate(packet)
      
      var test = parse(buf);
      console.log("////////TEST////////");
      console.log("Buffer: " + buf + ", Length: " + buf.length);
      console.log("Initial packet send to server: " + JSON.stringify(test));
      /*var changedbuf = buf.toString();
      var newbuf = new Buffer(changedbuf);
      var changedtest = parse(newbuf);
      console.log("Buffer(new): " + newbuf);
      console.log("After change packet send to server: " + JSON.stringify(changedtest));
      
      if(newbuf == buf){
         console.log("????????????????REALY THE SAME???????????????????????");
      }*/
      
    } catch(err) {
      req.sender.reset()
      return req.emit('error', err)
    }

    that._msgIdToReq[packet.messageId] = req
    that._tkToReq[that._lastToken] = req

    req.sender.send(buf)
  })

  req.sender = new RetrySend(this._sock, url.port, url.hostname || url.host)

  req.url = url

  req.statusCode = url.method || 'GET'

  urlPropertyToPacketOption(url, req, 'pathname', 'Uri-Path', '/')
  urlPropertyToPacketOption(url, req, 'query', 'Uri-Query', '&')

  if (options) {
    for (option in options) {
      if (options.hasOwnProperty(option)) {
        req.setOption(option, options[option])
      }
    }
  }

  if (url.proxyUri) {
    req.setOption('Proxy-Uri', url.proxyUri)
  }

  req.sender.on('error', req.emit.bind(req, 'error'))

  req.sender.on('sending', function() {
    that._msgInFlight++
  })

  req.sender.on('sent', function() {
    that._msgInFlight--
    if (that._closing && that._msgInFlight === 0) {
      that._doClose()
    }
  })

  if (url.observe)
    req.setOption('Observe', null)
  else
    req.on('response', this._cleanUp.bind(this))

  this._requests++

  req._totalPayload = new Buffer(0) 

  return req
}

function urlPropertyToPacketOption(url, req, property, option, separator) {
  if (url[property])
    req.setOption(option, url[property].split(separator)
         .filter(function(part) { return part !== '' })
         .map(function(part) {

      var buf = new Buffer(Buffer.byteLength(part))
      buf.write(part)
      return buf
    }))
}

module.exports = Agent
