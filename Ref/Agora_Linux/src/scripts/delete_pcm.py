#! /usr/bin/python
#! -*- coding: utf-8 -*-

import os
import stat
import syslog
import time
import traceback

outdated = []
path = "/data/pstn"
syslog.syslog(syslog.LOG_INFO, ": erase outdated pcms")

try:
  for f in os.listdir(path):
    s = os.stat(path + "/" + f)
    now = time.time()
    if stat.S_ISREG(s.st_mode) and s.st_mtime + 24 * 3600 < now:
      outdated.append(f)

  for f in outdated:
    syslog.syslog(syslog.LOG_INFO, "delete %s" %f)
    os.unlink(path + "/" + f)
except Exception, e:
  syslog.syslog(syslog.LOG_ERR, traceback.format_exc())

