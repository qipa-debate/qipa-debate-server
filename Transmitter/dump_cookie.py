import time
import sys
import pickle
from selenium import webdriver

if len(sys.argv) > 1:
	driver = webdriver.Chrome(sys.argv[1])  # Optional argument, if not specified will search path.
	#driver.add_cookie({'name' : 'session', 'value' : 'eyJfaWQiOnsiIGIiOiJaalUyTm1aak5EVTRPRE5qWWpVNU9HWmtNalZoWmpBNE4yUTNaR016TkRrPSJ9fQ', 'path' : '/'})

	driver.get('https://www.shimo.im/')
	time.sleep(60)
	print "Begin to dump!!!"
	cookies = driver.get_cookies()
	print cookies
	pickle.dump( driver.get_cookies() , open("cookies.pkl","wb"))
	print "dump finish"
	driver.quit()