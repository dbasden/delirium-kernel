#include <delirium.h>
#include <assert.h>
#include <ipc.h>
#include <rant.h>
#include "cpu.h"

#include "klib.h"

#include "i386/interrupts.h"
#include "i386/pic.h"
#include "i386/io.h"

#include "kthump.h"

/* 
 * Define to get around the problem that most keyboards don't
 * behave as we naievely expect. Will default to mode 2.
 */
#define KTHUMP_DONT_INIT_KEYBOARD
#define KTHUMP_DONT_INIT_KEYBOARD_CONTROLLER

/*
 * Kernel listener for keyboard events
 *
 *
 * TODO: Move all the scary  PC dependent stuff outta here
 */

char keymap[256] = KEYMAP;
char keymap_shift[256] = KEYMAP_SHIFT;
char keymap_caps[256] = KEYMAP_CAPS;

static bool shift = false;
static bool ctrl = false;
static bool alt = false;
static bool num = false;
static bool scroll = false;
static bool caps = false;

static soapbox_id_t key_soap;

static inline void scancode_rx(int scancode) {
	int key;
#ifdef ENABLE_GDB_STUB
	extern void breakpoint();
#endif
	
	/* Ignore all but the low byte */
	scancode = scancode & 0xff;

	/* Ignore extended scancodes for now */
	switch (scancode) {
	case KB_EXTENDA: kprint("[EA]"); return;
	case KB_EXTENDB: kprint("[EB]"); return;
	case KB_EXTENDC: kprint("[EC]"); return;
	}

	/* Check releases only for shift, alt and ctrl */
	if (scancode & KBM_RELEASE) {
		scancode = scancode ^ KBM_RELEASE;

		key = keymap[scancode] & 0xff;
		switch (key) {
		case KB_SHIFT:	shift = false;	break;
		case KB_ALT:	alt = false;	break;
		case KB_CTRL:	ctrl = false;	break;
		}
		return;
	}

	/* Translate into ASCII based on state */
	if (shift)	key = keymap_shift[scancode] & 0xff;
	else if (caps)	key = keymap_caps[scancode] & 0xff;
	else 		key = keymap[scancode] & 0xff;

	switch (key) {
		case KB_ERR: kprint("[kberr]"); break;
		case KB_SHIFT:	shift = true;	break;
		case KB_ALT:	alt = true;	break;
		case KB_CTRL:	ctrl = true;	break;
		case KB_NUM:	num = !num;	break;
		case KB_SCROLL:	scroll = !scroll; break;
		case KB_CAPS:	caps = !caps;	break;
#ifdef ENABLE_GDB_STUB
		case '~':	breakpoint();	break;
#endif
		default:	 
			{
				message_t msg;

				msg.type = signal;
				msg.m.signal = key;
				rant(key_soap, msg);
			}
	}
}


/* 
 * write to the keyboard controller (0x64)
 */
static inline void writeKBController(unsigned char c) { outb(0x64, c); }
/* 
 * read from the keyboard controller (0x64)
 */
static inline unsigned char readKBController() {
	unsigned char readChar;
	readChar = inb(0x64);
	return readChar;
}


/* 
 * Read from 0x60 after checking keyboard controller command byte. 
 * pre: NOT called from an interrupt context due to yield()
 */
static inline unsigned char readKB() {
  int spin;
  unsigned char readChar;

  for (spin=500; spin > 0; spin--) {
	  if (readKBController() & 0x01) {
	  	readChar = inb(0x60);
		return readChar;
	  }
    	  yield();
  }

  return 0xff; /* Fail read */
}


/*
 * write to port 0x60, after checking that OBF is clear
 */
static inline void writeKB(unsigned char c) {
	int spin;

	/* Wait for the controller to be ready to receive */
	for (spin = 500; spin > 0; spin--) {
	  if ((readKBController() & 0x02) == 0) {
		  outb(0x60, c);
		  return;
	  }
	  yield();
	}


  	kprint("[!!!!kbwritefail!!!!]");
}

static inline void check_kb_ack() {
	unsigned int response;

	response = readKB();
	if (response != 0xfa) {
		kprintf("Didn't get KBACK (got 0x%x)\n", response);
		return;
	}
}

static inline void writeKBwithACK(unsigned char c) {
	writeKB(c);
	check_kb_ack();
}


static inline void setKBControllerReg(unsigned char cmd) {
	/* Bit 6: (64) Translate	0 = Off. 1 = on
	 * Bit 5: (32) Mouse Enable
	 * 0 = 11 bit codes, check parity and do scan conversion
	 * 1 = use 8086 codes, don't check parity and don't do scan conversion
	 * Bit 4: (16) Keyboard enable 0 = Enable keyboard, 1 = Disable keyboard
	 * Bit 3: (8) Ignore keyboard lock	
	 * 0 = Don't ignore, 1: Force bit 4 of SR to 1 ("not locked")
	 * Bit 2: (4) System flag. 0 = "Cold reboot", 1 = "Warm reboot"
	 * Bit 1: (2) Mouse interrupt enable: Ugused excep on EISA/PS2
	 * Bit 0: (1) Keyboard interrupt enable: 
	 * 0 = No interrupts (polling). 1 = Use IRQ1 when output buffer full 
	 */

	writeKBController(0x60);
	writeKB(cmd);
}

static inline void emptyKBbuf() {
	unsigned char ignored;
	while (readKBController() & 0x01)
		ignored = readKB();
}

/*
 * Beware: Called from interrupt context.
 *         CANNOT use the above keyboard helpers, as they call yield() a lot
 */
void atKeyboardISR() {
	scancode_rx(inb(0x60));
}

void setup_kthump() {
	unsigned char cmd = 0;
	int kbtype = 0;
	int i;

#define KB_AT	0

#ifndef KTHUMP_DONT_INIT_KEYBOARD_CONTROLLER
	/*
	 * stop the keyboard controller from sending interrupts,
	 * and disable the keyboard while testing the controller
	 */
	setKBControllerReg((1 << 6) | (1 << 4) | (1 << 3) | (1 << 2));
	emptyKBbuf();

	/* Keyboard controller test */
	writeKBController(0xaa);
	for (i=0; i<10; i++) yield();
	cmd = readKB();
	if (cmd != 0x55) {
		//kprintf("\nKeyboard controller self test failed (0x%x)\n", cmd);
		kprint("[kbc!]");
	//	return;
	}

	writeKBController(0xab);
	yield();
	cmd = readKB();
	if (cmd != 0x00) {
		//kprintf("\nKeyboard to controller test failed (0x%x)\n", cmd);
		kprint("[kb2c!]");
	//	return;
	}

	/* Enable the keyboard clock*/
	writeKBController(0xae);
#endif
#ifndef KTHUMP_DONT_INIT_KEYBOARD

	/*
	 * Reset keyboard
	 */
	writeKBwithACK(0xff);
	cmd = readKB();
	if (cmd != 0xaa) {
		//kprintf("\nKeyboard self test failed (got 0x%x)\n", cmd);
		kprint("[kb!]");
		//return;
	}

	/* Figure out what type of keyboard it is */
	writeKBwithACK(0xf2);
	cmd = readKB();
	if (cmd == 0xff) {
		kbtype = KB_AT;
		kprint("(at)");
	} else {
		kbtype = cmd << 8;
		kbtype |= (readKB() & 0xff);
		kprintf("(mf:0x%4x)",kbtype);
	}

	writeKBwithACK(0xf5); /* Disable keyboard scanning */

	/* Switch off kb leds */
	writeKBwithACK(0xed);
	writeKBwithACK(0x00);

	/* Switch on numlock led */
	writeKBwithACK(0xed);
	writeKBwithACK(0x02);

	if (kbtype != KB_AT) {
		/* Try to set the keyboard to mode 3 */
		writeKBwithACK(0xf0);
		writeKBwithACK(0x03);

		writeKBwithACK(0xf0);
		writeKBwithACK(0x00);
		yield();
		cmd = readKB();

		switch (cmd) {
			case (1):
			case(0x43): kprint("(mode 1)"); break;
			case (2):
			case(0x41): kprint("(mode 2)"); break;
			case (3):
			case(0x3f): kprint("(mode 3)"); break;
			default:;
		}
	}

	writeKBwithACK(0xf4); /* Enable keyboard scanning */
#endif

	key_soap = get_new_soapbox("/hardware/keyboard");
	add_c_isr(INT_KEYBOARD, &atKeyboardISR);
	pic_unmask_interrupt(INT_KEYBOARD);

	/* Enable keyboard interrupts */
	setKBControllerReg((1 << 6) |(1 << 3) | (1 << 2) | 1);
}
