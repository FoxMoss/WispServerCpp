const bindings = require('bindings')
const wisp = bindings('wispservercpp')

wisp.Open(0, (msg) => {
  const encoded = Buffer.from(msg).toString('hex');
  console.log(encoded)
});
