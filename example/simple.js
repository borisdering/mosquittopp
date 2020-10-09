let some = require('../index');

let client = some.connect('mqtt://localhost:1883');

client.subscribe("test/test");

client.on('message', (message) => {
    console.log(`did receive a message ${message}`);
    client.publish('test', 'test', null, () => {
        console.log('done publish');
    });
});