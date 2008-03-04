#! /usr/bin/python

import sys

class Keymap(dict):
	def __init__(self):
		self.update(dict.fromkeys(range(256)))
		for k in self.keys():
			self[k] = ord('?')

	def parseLine(self, line):
		line = line.strip()
		if len(line) == 0: return
		if line[0] == '#': return
		pos = line.find(':')
		if pos == -1: return
		left = line[:pos]
		right = line[pos+1:]
		right = right.replace("<space>", ' ')
		right = right.replace("<bslash>", '\\')
		right = right.replace("<enter>", '\n')
		right = right.replace("<tab>", '\t')
		right = right.replace("<esc>", chr(0x1b))
		right = right.replace("<backspace>", chr(0x08))
		right = right.replace("<shift>", chr(0xd0))
		right = right.replace("<lshift>", chr(0xd0))
		right = right.replace("<rshift>", chr(0xd0))
		right = right.replace("<lctrl>", chr(0xd1))
		right = right.replace("<rctrl>", chr(0xd1))
		right = right.replace("<lalt>", chr(0xd2))
		right = right.replace("<ralt>", chr(0xd2))
		right = right.replace("<num>", chr(0xd3))
		right = right.replace("<scroll>", chr(0xd4))
		right = right.replace("<caps>", chr(0xd5))
		right = right.replace("<sysreq>", chr(0xd6))
		right = right.replace("<printscr>", chr(0xd6))
		right = right.replace("<up>", chr(0xd7))
		right = right.replace("<down>", chr(0xd8))
		right = right.replace("<left>", chr(0xd9))
		right = right.replace("<right>", chr(0xda))
		right = right.replace("<pause>", chr(0xdb))
		right = right.replace("<ins>", chr(0xdc))
		right = right.replace("<del>", chr(0xdd))
		right = right.replace("<home>", chr(0xde))
		right = right.replace("<end>", chr(0xdf))
		right = right.replace("<pgup>", chr(0xc0))
		right = right.replace("<pgdn>", chr(0xc1))

		right = right.replace("<f1>", chr(0xf1))
		right = right.replace("<f2>", chr(0xf2))
		right = right.replace("<f3>", chr(0xf3))
		right = right.replace("<f4>", chr(0xf4))
		right = right.replace("<f5>", chr(0xf5))
		right = right.replace("<f6>", chr(0xf6))
		right = right.replace("<f7>", chr(0xf7))
		right = right.replace("<f8>", chr(0xf8))
		right = right.replace("<f9>", chr(0xf9))
		right = right.replace("<f10>", chr(0xfa))
		right = right.replace("<f11>", chr(0xfb))
		right = right.replace("<f12>", chr(0xfc))

		if (left[:2] =="0x"): 
			left = int(left[2:],16)
		else: 
			left = int(left)

		while len(right):
			self[left] = ord(right[0])
			left = left + 1
			right = right[1:]
	
	def parseFile(self, f):
		for l in f.readlines():
			self.parseLine(l)

	def __str__(self):
		return '{'+ ", ".join(map(str, map(self.get, range(256))))+'};'

if __name__ == "__main__":
	k = Keymap()
	k.parseFile(sys.stdin)
	print k
