import socket
import numpy as np

PORT = 12345
HOST = '127.0.0.1'
MAX_BYTE_SIZE = 65482

DMD_WIDTH_HEIGHT = (2716,1600)
arr = np.ones(DMD_WIDTH_HEIGHT, dtype=np.uint8)  # ROW MAJOR FORMAT
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST,PORT))


def connection_test():
	test_arr = np.zeros(DMD_WIDTH_HEIGHT, dtype=np.uint8)  # ROW MAJOR FORMAT
	for i in range(DMD_WIDTH_HEIGHT[0]):
		test_arr[i, :] = i % 255
	s.sendall(test_arr.tobytes())



connection_test()
