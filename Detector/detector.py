import os
import sys
import time
import threading
import Pyro4
import commands

file_path = sys.argv[1]
file_list = []

class Detector:
	def __init__(self):
		self.lock = threading.RLock()
		self.words = ""
		self.content = ""

	def get_content(self):
		with self.lock:
			content_t = self.content
			self.content = ""
		return content_t

	def get_words(self):
		with self.lock:
			words_t = self.words
			self.words = ""

	def set_content(self, c):
		with self.lock:
			self.content = self.content + "\n" + c
			print self.content

	def set_words(self, w):
		with self.lock:
			self.words = w
			print self.words

detector = Detector()

def run():
	while True:
		port = 8080
		try:
			daemon = Pyro4.Daemon("0.0.0.0", port)                # make a Pyro daemon
			uri = daemon.register(detector)   # register the greeting maker as a Pyro object

			print("Ready. Object uri =", uri)      # print the uri so we can use it in the client later
			daemon.requestLoop() 
		except:
			port = port + 1
			if port > 8090:
				port = 8080

thread = threading.Thread(target = run)
thread.setDaemon(True)
thread.start()

while True:
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
			with open(file_prefix + ".content", "r") as fp:
				detector.set_content(fp.read().decode("utf-8"))
			
			with open(file_prefix + ".words", "r") as fp:
				detector.set_words(fp.read().decode("utf-8"))

			file_list.append(temp_file[0][1])
	except Exception, e:
		print e
	time.sleep(5)


