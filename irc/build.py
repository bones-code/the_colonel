#!/usr/bin/env python
# ADD CONDITION THAT LOOKS FOR KEYMAP, IF NONE, THEN BUILD

# Builds the lists (maps) required by bot.py for keylog translations.
# All key names and indexes from linux/input.h.
from platform import release
import linecache
import fnmatch
import re


SPECIAL = ['RESERVED', 'ESC', 'BACKSPACE',  'CAPSLOCK', 'F1', 'F2', 'F3', 
'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10', 'NUMLOCK', 'SCROLLLOCK', 'F11',
'F12', 'SYSRQ', 'LINEFEED', 'UP', 'PAGEUP', 'LEFT', 'RIGHT', 'END', 'DOWN', 
'PAGEDOWN', 'INSERT', 'DELETE', 'MACRO', 'MUTE', 'VOLUMEDOWN', 'VOLUMEUP', 
'POWER', 'PAUSE', 'SCALE', 'HANGEUL', 'COMPOSE', 'STOP', 'AGAIN', 'PROPS', 
'UNDO', 'FRONT', 'COPY', 'OPEN', 'PASTE', 'FIND', 'CUT', 'HELP', 'MENU', 
'CALC', 'SETUP', 'SLEEP', 'WAKEUP', 'FILE', 'SENDFILE', 'DELETEFILE', 'XFER', 
'PROG1', 'PROG2', 'WWW', 'MSDOS', 'COFFEE', 'SCREENLOCK', 'DIRECTION', 
'CYCLEWINDOWS', 'MAIL', 'BOOKMARKS', 'COMPUTER', 'BACK', 'FORWARD', 
'CLOSECD', 'EJECTCD', 'EJECTCLOSECD', 'HOME','NEXTSONG', 'PLAYPAUSE', 
'PREVIOUSSONG', 'STOPCD', 'RECORD', 'REWIND', 'PHONE', 'ISO', 'CONFIG', 
'HOMEPAGE', 'REFRESH', 'EXIT', 'MOVE','EDIT', 'SCROLLUP', 'SCROLLDOWN', 
'NEW', 'REDO', 'F13', 'F14', 'F15', 'F16', 'F17', 'F18', 'F19', 'F20', 'F21', 
'F22', 'F23', 'F24', 'PLAYCD', 'PAUSECD', 'PROG3', 'PROG4', 'DASHBOARD', 
'SUSPEND', 'CLOSE', 'PLAY', 'FASTFORWARD', 'BASSBOOST', 'PRINT', 'HP', 
'CAMERA', 'SOUND', 'QUESTION', 'EMAIL', 'CHAT', 'SEARCH', 'CONNECT', 
'FINANCE', 'SPORT', 'SHOP', 'ALTERASE', 'CANCEL', 'BRIGHTNESSDOWN', 
'BRIGHTNESSUP', 'MEDIA', 'SWITCHVIDEOMODE', 'KBDILLUMTOGGLE', 'KBDILLUMDOWN', 
'KBDILLUMUP', 'SEND', 'REPLY', 'FORWARDMAIL', 'SAVE', 'DOCUMENTS', 'BATTERY', 
'BLUETOOTH', 'WLAN', 'UWB', 'UNKNOWN', 'VIDEO_NEXT', 'VIDEO_PREV', 
'BRIGHTNESS_CYCLE', 'BRIGHTNESS_ZERO', 'DISPLAY_OFF', 'WIMAX', 'RFKILL']

# Dictionary of lowercase letters, numbers, punctuation, and 
# function keys for keymap listing.
SYM = {'KPLEFTPAREN': '(', 'KPRIGHTPAREN': ')','ENTER': '[ENTER]', 
'KPENTER': '[ENTER]', 'TAB': ' [TAB]	', 'RIGHTCTRL': '[CTRL]', 
'LEFTCTRL': '[CTRL]', 'MINUS': '-', 'EQUAL': '=', 'LEFTBRACE': '[', 
'RIGHTBRACE': ']', 'SEMICOLON': ';', 'APOSTROPHE': "'", 'GRAVE': '`', 
'RIGHTALT': '[ALT]', 'BACKSLASH': '[BACKSLASH]', 'COMMA': ',', 'DOT': '.', 
'SLASH': '/', 'KPASTERISK': '*', 'SPACE': ' ', 'KP7': '7', 'KP8': '8', 
'KP9': '9', 'KPMINUS': '-', 'KP4': '4', 'KP5': '5', 'KP6': '6', 
'KPPLUS': '+', 'KP1': '1', 'KP2': '2', 'KP3': '3', 'KP0': '0', 
'KPDOT': '.', 'KPSLASH': '/', 'KPEQUAL': '=', 'KPPLUSMINUS': '+-', 
'KPCOMMA': ',', 'LEFTMETA': 'COMMAND', 'RIGHTMETA': 'COMMAND', 
'LEFTSHIFT': '[SHIFT]', 'RIGHTSHIFT': '[SHIFT]', 'LEFTALT': '[ALT]', 
'Q': 'q', 'W': 'w', 'E': 'e', 'R': 'r', 'T': 't', 'Y': 'y', 'U': 'u', 
'I': 'i', 'O': 'o', 'P': 'p', 'A': 'a', 'S': 's', 'D': 'd', 'F': 'f', 
'G': 'g', 'H': 'h', 'J': 'j', 'K': 'k', 'L': 'l', 'Z': 'z', 'X': 'x', 
'C': 'c', 'V': 'v', 'B': 'b', 'N': 'n', 'M': 'm'}

# Dictionary of numbers, punctuation, and 
# function keys for cap_keymap listing.
CAP_SYM = {'KPLEFTPAREN': '(', 'KPRIGHTPAREN': ')', 'ENTER': '[ENTER]', 
'KPENTER': '[ENTER]', 'TAB': ' [TAB]	', 'RIGHTCTRL': '[CTRL]', 
'LEFTCTRL': '[CTRL]', 'MINUS': '-', 'EQUAL': '=', 'LEFTBRACE': '[', 
'RIGHTBRACE': ']', 'SEMICOLON': ';', 'APOSTROPHE': "'",'GRAVE': '`', 
'RIGHTALT': '[ALT]','BACKSLASH': '[BACKSLASH]', 'COMMA': ',', 'DOT': '.', 
'SLASH': '/', 'KPASTERISK': '*', 'SPACE': ' ', 'KP7': '7', 'KP8': '8', 
'KP9': '9', 'KPMINUS': '-', 'KP4': '4', 'KP5': '5', 'KP6': '6', 'KPPLUS': '+', 
'KP1': '1', 'KP2': '2', 'KP3': '3', 'KP0': '0', 'KPDOT': '.', 'KPSLASH': '/', 
'KPEQUAL': '=', 'KPPLUSMINUS': '+-', 'KPCOMMA': ',', 'LEFTMETA': 'COMMAND', 
'RIGHTMETA': 'COMMAND', 'LEFTSHIFT': '[SHIFT]', 'RIGHTSHIFT': '[SHIFT]', 
'LEFTALT': '[ALT]'}

# Dictionary of lowercase letters, numbers, punctuation, and 
# function keys for cap_shift_keymap listing.
CAP_SHIFT_SYM = {'KPLEFTPAREN': '(', 'KPRIGHTPAREN': ')', 'ENTER': '[ENTER]', 
'KPENTER': '[ENTER]', 'TAB': ' [TAB]	', 'RIGHTCTRL': '[CTRL]', 
'LEFTCTRL': '[CTRL]', 'MINUS': '_', 'EQUAL': '+', 'LEFTBRACE': '{', 
'RIGHTBRACE': '}', 'SEMICOLON': ':', 'APOSTROPHE': '\\\"', 'GRAVE': '~', 
'RIGHTALT': '[ALT]', 'BACKSLASH': '|', 'COMMA': '<', 'DOT': '>', 
'SLASH': '?', 'KPASTERISK': '*', 'SPACE': ' ', 'KP7': '7', 'KP8': '8', 
'KP9': '9', 'KPMINUS': '-', 'KP4': '4', 'KP5': '5', 'KP6': '6', 
'KPPLUS': '+', 'KP1': '1', 'KP2': '2', 'KP3': '3', 'KP0': '0', 'KPDOT': '.', 
'KPSLASH': '/', 'KPEQUAL': '=', 'KPPLUSMINUS': '+-', 'KPCOMMA': ',', 
'LEFTMETA': 'COMMAND', 'RIGHTMETA': 'COMMAND', 'LEFTSHIFT': '[SHIFT]', 
'RIGHTSHIFT': '[SHIFT]', 'LEFTALT': '[ALT]', '1': '!', '2': '@', '3': '#', 
'4': '$', '5': '%', '6': '^', '7': '&', '8': '*', '9': '(', '0': ')',
'Q': 'q', 'W': 'w', 'E': 'e', 'R': 'r', 'T': 't', 'Y': 'y', 'U': 'u', 
'I': 'i', 'O': 'o', 'P': 'p', 'A': 'a', 'S': 's', 'D': 'd', 'F': 'f', 
'G': 'g', 'H': 'h', 'J': 'j', 'K': 'k', 'L': 'l', 'Z': 'z', 'X': 'x', 
'C': 'c', 'V': 'v', 'B': 'b', 'N': 'n', 'M': 'm'}

# Dictionary of numbers, punctuation, and 
# function keys for shift_keymap listing.
SHIFT_SYM = {'KPLEFTPAREN': '(', 'KPRIGHTPAREN': ')', 'ENTER': '[ENTER]', 
'KPENTER': '[ENTER]', 'TAB': ' [TAB]	', 'RIGHTCTRL': '[CTRL]', 
'LEFTCTRL': '[CTRL]', 'MINUS': '_', 'EQUAL': '+', 'LEFTBRACE': '{', 
'RIGHTBRACE': '}', 'SEMICOLON': ':', 'APOSTROPHE': '\\\"', 'GRAVE': '~', 
'RIGHTALT': '[ALT]', 'BACKSLASH': '|', 'COMMA': '<', 'DOT': '>', 
'SLASH': '?', 'KPASTERISK': '*', 'SPACE': ' ', 'KP7': '7', 'KP8': '8', 
'KP9': '9', 'KPMINUS': '-', 'KP4': '4', 'KP5': '5', 'KP6': '6', 
'KPPLUS': '+', 'KP1': '1', 'KP2': '2', 'KP3': '3', 'KP0': '0', 'KPDOT': '.', 
'KPSLASH': '/', 'KPEQUAL': '=', 'KPPLUSMINUS': '+-', 'KPCOMMA': ',', 
'LEFTMETA': 'COMMAND', 'RIGHTMETA': 'COMMAND', 'LEFTSHIFT': '[SHIFT]', 
'RIGHTSHIFT': '[SHIFT]', 'LEFTALT': '[ALT]', '1': '!', '2': '@', '3': '#', 
'4': '$', '5': '%', '6': '^', '7': '&', '8': '*', '9': '(', '0': ')'}

# keyed function shapes the specified keymap.
def keyed(pyf, km, sym_list):
	for s in km:
		if s == km[len(km)-1]:
			last = '"[' + s + ']"]'
			pyf.write(last)
			break
		elif s in SPECIAL:
			btn = '"[' + s + ']",\n'
			pyf.write(btn)
		elif s in sym_list:
			punc = '"' + sym_list[s] + '",\n'
			pyf.write(punc)
		else:
			entry = '"' + s + '", \n'
			pyf.write(entry)
	return km

# Builds key list from linux/input.h. The KEY_ prefix is stripped. 
# The index of each key matches its respective keycode.
def key_list(km, ckm, cskm, skm):
	for i in range(183, 442):
		linput = linecache.getline("/usr/src/linux-headers-" + release() + "/include/linux/input.h", i).split()
		key = fnmatch.filter(linput, 'KEY*')
		try:
			key = re.sub(r'KEY_', "", key[0])
			km.append(key)
			ckm.append(key)
			cskm.append(key)
			skm.append(key)
		except:
			continue

def main():
	keymap = []
	cap_keymap = []
	cap_shift_keymap = []
	shift_keymap = []

	key_list(keymap, cap_keymap, cap_shift_keymap, shift_keymap)

	# Creates keymap.py file to write maps to.
	f = open("keymap.py", "w")

	# Builds character keymap.
	f.write("keys = [\n")
	keyed(f, keymap, SYM)

	# Builds capslock keymap.
	f.write("\n\ncap_keys = [\n")
	keyed(f, cap_keymap, CAP_SYM)

	# Builds capslock/shift combination keymap.
	f.write("\n\ncap_shift_keys = [\n")
	keyed(f, cap_shift_keymap, CAP_SHIFT_SYM)

	# Builds shift keymap.
	f.write("\n\nshift_keys = [\n")
	keyed(f, shift_keymap, SHIFT_SYM)

	f.close()

if __name__ == '__main__':
	main()
