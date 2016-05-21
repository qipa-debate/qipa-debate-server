import time
import datetime
import sys
import pickle
from selenium import webdriver
from selenium.webdriver.common.action_chains import ActionChains

if len(sys.argv) > 1:
	driver = webdriver.Chrome(sys.argv[1])  # Optional argument, if not specified will search path.
	#driver.add_cookie({'name' : 'session', 'value' : 'eyJfaWQiOnsiIGIiOiJaalUyTm1aak5EVTRPRE5qWWpVNU9HWmtNalZoWmpBNE4yUTNaR016TkRrPSJ9fQ', 'path' : '/'})
	driver.implicitly_wait(0)
	driver.get('https://www.shimo.im/')
	cookies = pickle.load(open("cookies.pkl", "rb"))
	for cookie in cookies:
		driver.add_cookie(cookie)
	driver.get('https://www.shimo.im/doc/g8XvnVEvCtEWnvrB')
	search_box = driver.find_element_by_class_name('locate')
	mouse = ActionChains(driver)
	mouse.move_to_element_with_offset(search_box, 0, 0)
	mouse.click()
	mouse.send_keys(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) + ': BBB\n')
	mouse.perform()
	time.sleep(5)
	driver.quit()