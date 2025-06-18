# --------------------------------------------------------------------------
# 
# --------------------------------------------------------------------------
import time, os, sys, struct, datetime
from serial_slip import Serial_SLIP
import argparse
# --------------------------------------------------------------------------

# Constants for SLIP commands
CMD_NONE = 0,
CMD_RSP_ERROR = 1
CMD_REQ_PING = 2
CMD_RSP_PONG = 3
CMD_LOG = 4
CMD_WRITE_MEM = 5
CMD_START_EMU = 6
CMD_STOP_EMU = 7
CMD_STEP_EMU = 8
CMD_GET_INST_COUNT = 9

last_inst_count = 0
def receive_cb():
  global last_inst_count
  while(dev.in_wait()):
    data = dev.get()
    if(len(data) == 0):
      continue
    tag, data = data[0], data[1:]
    if(tag == CMD_RSP_ERROR):
      print("Error: ", data)
    elif(tag == CMD_RSP_PONG):
      print("Pong received")
    elif(tag == CMD_LOG):
      dt = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
      print(f"[{dt}][Log]: ", data.decode('utf-8'), end = "")
    elif(tag == CMD_GET_INST_COUNT):
      inst_count = struct.unpack("<I", data)[0]
      print(f"Instruction per second: {inst_count - last_inst_count} (Total: {inst_count})")
      last_inst_count = inst_count
    else:
      print("Unknown command received:", tag)

# --------------------------------------------------------------------------
if(__name__ == "__main__"):
  parser = argparse.ArgumentParser(description="BitBoard6502 Serial Interface")
  parser.add_argument("command", type=str, nargs="?", default="ping",
                      choices=["ping", "write", "start", "stop", "step"],
                      help="Command to execute")
  parser.add_argument("-p", "--port", required=True, type=str, 
                      help="Serial port to connect to")
  parser.add_argument("-b", "--baudrate", type=int, default=115200, 
                      help="Baud rate for serial communication")
  parser.add_argument("-f", "--file", type=str, default=None, 
                      help="File to load into the emulator (optional)")
  parser.add_argument("-a", "--write_address", type=int, default=0x8000,
                      help="Write address (default: 0x8000)")
  args = parser.parse_args()
  # --------------------------------------------------------------------------

  dev = Serial_SLIP(args.port, checksum_enable = False)
  dev.set_receive_callback(receive_cb)
 
  match args.command:
    case "ping":
      print("Sending ping command...")
      dev.write(CMD_REQ_PING)
      dev.write_end()
    case "write":
      if args.file is None:
        print("Error: No file specified for writing to memory.")
        sys.exit(1)
      if not os.path.isfile(args.file):
        print(f"Error: File '{args.file}' does not exist.")
        sys.exit(1)

      print(f"Writing file '{args.file}' to memory at reset vector {hex(args.write_address)}...")
      maxpacket_size = 256  # Maximum packet size for writing
      with open(args.file, "rb") as f:
        i = 0
        while True:
          data = f.read(maxpacket_size)  # Read up to 512 bytes
          if not data:
            break
          # if data full of 0, dont send it
          if all(byte == 0 for byte in data):
            #print(f"Skipping writing {len(data)} bytes at address {hex(args.write_address + i)} (all zeros)")
            i += maxpacket_size
            continue
          dev.write(CMD_WRITE_MEM)
          print(f"Writing {len(data)} bytes to address {hex(args.write_address + i)}")
          dev.write(struct.pack("H", args.write_address + i))  # Write write_address
          dev.write(data)  # Write file data
          dev.write_end()
          i += maxpacket_size
          time.sleep(0.5)  # Sleep to avoid overwhelming the device
    case "start":
      print("Starting emulator...")
      dev.write(CMD_START_EMU)
      dev.write_end()
    case "stop":
      print("Stopping emulator...")
      dev.write(CMD_STOP_EMU)
      dev.write_end()
    case "step":
      print("Stepping emulator...")
      dev.write(CMD_STEP_EMU)
      dev.write_end()
  
  #...
  last_inst_count_time = 0
  while(1):
    # Check if the command is "step" to allow stepping through the emulator
    if(args.command == "step"):
      input("Press Enter to step again...")
      dev.write(CMD_STEP_EMU)
      dev.write_end()
    # Check for incoming data

    if (time.time() - last_inst_count_time > 1 and 1):
      dev.write(CMD_GET_INST_COUNT)
      dev.write_end()
      last_inst_count_time = time.time()
    
    time.sleep(0.1)