import bindings from 'bindings';
import express from 'express';
import { WebSocketServer } from 'ws';
const wisp = bindings('wispservercpp')

const wsServer = new WebSocketServer({ noServer: true });

const socketMaps = {};

const sendCallback = (exit, msg, id) => {
  let socket = socketMaps[id]
  socket.send(msg)
  if (exit) {
    socket.close()
  }
}


wsServer.on('connection', socket => {
  let id = wisp.NextID();
  socketMaps[id] = socket;
  wisp.Open(id, sendCallback)

  socket.on('message', message => wisp.Message(id, message.toString(), sendCallback));
});

export const routeUpgrade = (req, socket, head) => {
  wsServer.handleUpgrade(req, socket, head, socket => {
    wsServer.emit('connection', socket, req);
  });
}


wisp.Init(sendCallback);

console.log("Runnning Wisp Server on 6001");
const app = express();
const server = app.listen(6001);
server.on('upgrade', (request, socket, head) => {
  routeUpgrade(request, socket, head)
});
