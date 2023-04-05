import usb.core
import sys
import time

Endpoint_in_idx  = 0
Endpoint_out_idx = 0

dev = usb.core.find(idVendor = 0x2e8a, idProduct=0x000a)

if dev == None:
    print("No USB device found!")
    sys.exit(0)

# Get Configuration
cfg = dev.get_active_configuration()

# Get Interface
intf = cfg[(0,0)] # (index, alternate settings index)

# Search for the index of the EP_OUT
for i, ep in enumerate(intf.endpoints()):
     if ep.bEndpointAddress == (0x01 | usb.util.ENDPOINT_OUT):
        Endpoint_out_idx = i
     elif ep.bEndpointAddress == (0x81 | usb.util.ENDPOINT_IN):
        Endpoint_in_idx = i

# Get OUT Endpoint handler
ep_out = intf[Endpoint_out_idx]

# Get IN Endpoint handler
ep_in = intf[Endpoint_in_idx]


print(ep_in)
print(ep_out)
print("")

# Set up data
data = bytearray(4)
for i in range(4):
    data[i] = i * 20

# Write to OUT EP
ep_out.write(data, timeout=1000)

#
print(f"Sent data ({len(data)}-byte):")
for i in range(0, len(data), 32):
    line_data = " ".join(f"{byte:02X}" for byte in data[i:i+32])
    print(line_data)

# display new line
print("")

# Read data (up to 64 bytes) from the IN endpoint
try:
    ReceivedData = ep_in.read(ep_in.wMaxPacketSize, timeout=1000)
except usb.core.USBError as e:
    print(f"Error while reading from IN endpoint: {e}")
    # There is no data available from the device
    #pass
else:
    # Display the 64 bytes of data
    print(f"Received data ({len(ReceivedData)}-byte):")
    for i in range(0, len(ReceivedData), 32):
        line_data = " ".join(f"{byte:02X}" for byte in ReceivedData[i:i+32])
        print(line_data)

# Exit the script with a status of 0
sys.exit(0)
