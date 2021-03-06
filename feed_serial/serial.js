const SerialPort = require('serialport')

let serialPort;

// Gets the Serial Device Path for Our Pico
const getPicoPath = async () => {
    const devices = await SerialPort.list();
    return devices.find(device => device.manufacturer === 'Raspberry Pi').path;
}

// Async function that data (characters in our case) over the Serial Connection with a pause
// NOTE: This timeout is required so that the pico can actually read all the characters being passed
const _write = async (data) => {
    return new Promise(res => setTimeout(() => serialPort.write(data, res), 30));
}

// Async function that Sends a string as characters over the Serial Connection
// NOTE: The Pico only seems to like recieving 1 byte at a time within the given time frame. Even though the Serial Connection is configured for this
const write = async (data) => {
    for(const ch of data)
        await _write(ch);
}

// Async function that opens the Serial Connection with a Delay
const open = async () => {
    const devicePath = await getPicoPath();
    serialPort = new SerialPort(devicePath, {
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: "none",
        autoOpen: true
    });

    serialPort.on('close', (e) => {
        console.log(`Closed Serial Connection (Error ${!!e})`);
    });

    return new Promise(res => serialPort.open(() => setTimeout(res, 1000)));
}

module.exports = {
    open,
    write
};