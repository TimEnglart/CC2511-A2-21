const SerialPort = require('serialport')

const serialport = new SerialPort('/dev/ttyACM0', {
    baudRate: 115200,
    dataBits: 8,
    stopBits: 1,
    parity: "none",
    autoOpen: true
});

// Async function that data (characters in our case) over the Serial Connection with a pause
// NOTE: This pause is required so that the pico can actually read all the characters being passed
const _write = async (data) => {
    return new Promise(res => setTimeout(() => serialport.write(data, res), 30));
}

// Async function that Sends a string as characters over the Serial Connection
// NOTE: The Pico only seems to like recieving 1 byte at a time within the given time frame
const write = async (data) => {
    for(const ch of data)
        await _write(ch);
}

// Async function that opens the Serial Connection with a Delay
const open = async () => {
    return new Promise(res => serialport.open(() => setTimeout(res, 1000)));
}

serialport.on('close', (e) => {
    console.log(`Closed Serial Connection (Error ${!!e})`);
});

module.exports = {
    open,
    write
};