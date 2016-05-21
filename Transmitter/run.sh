current=`pwd`
if [ ! -f "../Temp/chromedriver" ]; then
	unzip ../Ref/Selenium/chromedriver_mac32.zip -d ../Temp
	cd ../Ref/Selenium/selenium-2.53.2/
	sudo python setup.py install
	cd ${current}
fi

if [ ! -f "./cookies.pkl" ]; then
	python dump_cookie.py ../Temp/chromedriver
fi

python transmitter.py ../Temp/chromedriver

