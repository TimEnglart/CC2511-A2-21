'''
    Data Structure will be similar to csv
    Positions will be based on coordinates which the program will determine the number
    of steps to get the the provided coordinate.

    (Do we want to provide a mode override?)

    X_POSITION,Y_POSITION,Z_POSITION;X_POSITION,Y_POSITION,Z_POSITION; ... etc

    Example:
    Data Sent: 1,4,0;5,2,0;10,10,0;

    Data in Python: [
        [ 1, 4, 0 ],
        [ 5, 2, 0 ],
        [ 10, 10, 0 ],
    ]


Setup Instructions:
    python3 -m venv feed_serial

    Windows: tutorial-env\Scripts\activate.bat
    Unix: source tutorial-env/bin/activate

    pip install pyserial

    python feed_serial.py
'''

import serial

def prepare_data(data):
    for i in range(len(data)):
        data[i] =  ",".join(data[i])
    return ";".join(data)
        


class SerialWrapper:
    def __init__(self, device):
        self.ser = serial.Serial(device, 115200)

    def sendData(self, data):
        data += "\r\n"
        self.ser.write(data.encode())

def main():
    serial_device_name = input('Enter the Serial Device Name: ')
    ser = SerialWrapper(serial_device_name)
    ser.sendData(prepare_data())
