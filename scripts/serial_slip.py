from serial.threaded import ReaderThread, Protocol
import serial
import struct

#------------------------------------------------------------------------------
class Serial_SLIP(Protocol):
  END = 0xC0
  ESC = 0xDB
  ESC_END = 0xDC
  ESC_ESC = 0xDD
  def __init__(self, port: str, baudrate: int = 115200, checksum_enable: bool = True, timeout: float = 0.1):
    self.buffer: list[int] = []
    self.packages: list[bytearray] = []
    self.esc_flag: bool= False
    self.wait_ack: bool = False
    self.checksum_enable: bool = checksum_enable
    self.checksum = 0

    self.serial = serial.Serial(port, baudrate = baudrate, timeout=timeout)
    self.serial_thread = ReaderThread(self.serial, self._self_)
    self.serial_thread.start()

    self.receive_callback = None

  def set_receive_callback(self, callback):
    """
    Set a callback function to be called when data is received.
    The callback should accept a single argument, which will be the received data as a bytearray.
    """
    self.receive_callback = callback

  def _self_(self):
    return self
  # Called when data is received By Serial.ReaderThread
  def data_received(self, data: bytes):
    for byte in data:
      self.push(byte)

  def write(self, data: int|bytes|bytearray|list[int], check_checksum: bool = True):
    if(isinstance(data, bytes) or isinstance(data, list) or isinstance(data, bytearray)):
      for byte in data:
        self.write(byte)
    elif(isinstance(data, int)):
      self.checksum += (data + 1)*check_checksum
      if(data == self.END):
        self.serial.write(bytes([self.ESC, self.ESC_END]))
      elif(data == self.ESC):
        self.serial.write(bytes([self.ESC, self.ESC_ESC]))
      else:
        self.serial.write(bytes([data]))
    self.checksum %= 2**32

  # Sends the end of the SLIP packet
  # and the checksum to the serial32-CAM
  def write_end(self):
    if(self.checksum_enable):
      self.write(self.checksum & 0xFF, False)
      self.write((self.checksum >> 8) & 0xFF, False)
      self.write((self.checksum >> 16) & 0xFF, False)
      self.write((self.checksum >> 24) & 0xFF, False)
    self.checksum = 0
    self.serial.write(bytes([self.END]))
  #...
  def push(self, value: int):
    #...
    if(self.esc_flag):
      if(value == self.ESC_END):
        self.buffer.append(self.END)
        self.checksum += self.END + 1
      elif(value == self.ESC_ESC):
        self.buffer.append(self.ESC)
        self.checksum += self.ESC + 1
      elif(value == self.END):
        self.wait_ack = True
      self.esc_flag = False
    #...
    elif(value == self.ESC):
      self.esc_flag = True
    #...
    elif(value == self.END):
      if(self.checksum_enable):
        #...
        if(len(self.buffer) < 4):
          self.reset_buffer()
          return
        #...
        for byte in self.buffer[-4:]:
          self.checksum -= byte+1
        checksum = struct.unpack("I", bytes(self.buffer[-4:]))[0]
        #...
        if(self.checksum != checksum): 
          self.reset_buffer()
          return
        self.buffer = self.buffer[:-4]
      self.packages.append(bytearray(self.buffer))
      if self.receive_callback is not None:
        # Call the receive callback with the received data
        self.receive_callback()
      self.reset_buffer()
    #...
    else:
      self.buffer.append(value)
      self.checksum += value + 1
    self.checksum %= 2**32
  #...
  def get(self) -> bytearray:
    if(len(self.packages) > 0):
      return(self.packages.pop(0))
    return(bytearray())
  #...
  def in_wait(self) -> int:
    return(len(self.packages))
  #...
  def reset_buffer(self):
    self.buffer.clear()
    self.esc_flag = False
    self.wait_ack = False
    self.checksum = 0