import os
import sys
import time
import threading
import socket
import commands

file_path = sys.argv[1]
file_list = []
lock = threading.RLock()
words = ""
content = ""

def run():
	while True:
		try:
			s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
			s.bind(("0.0.0.0",8090))
			s.listen(1)
			while True:
				conn,addr = s.accept()
				print 'Connected by', addr
				data = conn.recv(1024)
				if data == "content":
					with lock:
						conn.sendall(len(content))
						conn.sendall(content)
						content = ""
				elif data == "words":
					with lock:
						conn.sendall(len(words))
						conn.sendall(words)
						words = ""
				conn.close()
		except Exception, e:
			print e
		finally:
			s.close()

thread = threading.Thread(target = run)
thread.setDaemon(True)
thread.start()

while True:
	print "%s, %s, %s" % (file_list, content, words)
	try:
		temp_file = []
		for root, _, files in os.walk(file_path):
			for file in files:
				if not file.endswith(".pcm") or file in file_list:
					continue

				temp_file.append((int(file.split(".")[0].split("_")[1]), file))
		
		print "%s, %s" % (temp_file, file_list)
		temp_file.sort()
		if len(temp_file) - len(file_list) >= 2:
			file_prefix = temp_file[0][1][:-4]
			print file_prefix

			print os.system("sh run.sh " + file_prefix)
			with lock:
				with open(file_prefix + ".content", "r") as fp:
					content = content + "\n" + fp.read().decode("utf-8")
					print content
				
				with open(file_prefix + ".words", "r") as fp:
					words = fp.read().decode("utf-8")
					print words

			file_list.append(temp_file[0][1])
	except Exception, e:
		print e
	time.sleep(5)


