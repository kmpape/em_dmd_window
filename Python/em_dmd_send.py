import socket
import time
import numpy as np

DEBUG = False
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

def sleep(duration: float):
	now = time.perf_counter()
	end = now + duration
	while (now < end):
		now = time.perf_counter()

connection_test()


def run_test(delay = 0.001, use_time=False):
	i = 0
	print("Enumerating dim 1")
	for j in range(DMD_WIDTH_HEIGHT[0]):
		if DEBUG:
			print(f"Image {i}")
		test_arr = np.zeros(DMD_WIDTH_HEIGHT, dtype=np.uint8)
		test_arr[j, :] = 255
		s.sendall(test_arr.transpose().tobytes())
		if use_time:
			time.sleep(delay)
		else:
			sleep(delay)
		i += 1
	
	print("Enumerating dim 2")
	for j in range(DMD_WIDTH_HEIGHT[1]):
		if DEBUG:
			print(f"Image {i}")
		test_arr = np.zeros(DMD_WIDTH_HEIGHT, dtype=np.uint8)
		test_arr[:, j] = 255
		s.sendall(test_arr.transpose().tobytes())
		if use_time:
			time.sleep(delay)
		else:
			sleep(delay)
		i += 1
	print("Finished enumeration.")

