// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

        Kramer MC driver by Miodrag Milanovic

        13/09/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/kramermc.h"

#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "screen.h"


static GFXDECODE_START( kramermc )
	GFXDECODE_ENTRY( "gfx1", 0x0000, kramermc_charlayout, 0, 1 )
GFXDECODE_END

/* Address maps */
void kramermc_state::kramermc_mem(address_map &map)
{
	map(0x0000, 0x03ff).rom();  // Monitor
	map(0x0400, 0x07ff).rom();  // Debugger
	map(0x0800, 0x0bff).rom();  // Reassembler
	map(0x0c00, 0x0fff).ram();  // System RAM
	map(0x1000, 0x7fff).ram();  // User RAM
	map(0x8000, 0xafff).rom();  // BASIC
	map(0xc000, 0xc3ff).rom();  // Editor
	map(0xc400, 0xdfff).rom();  // Assembler
	map(0xfc00, 0xffff).ram();  // Video RAM
}

void kramermc_state::kramermc_io(address_map &map)
{
	map.global_mask(0xff);
	map(0xfc, 0x0ff).rw("z80pio", FUNC(z80pio_device::read), FUNC(z80pio_device::write));
}

/* Input ports */
static INPUT_PORTS_START( kramermc )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl")  PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(">") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
	PORT_START("LINE7")
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/* Machine driver */
MACHINE_CONFIG_START(kramermc_state::kramermc)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 1500000)
	MCFG_CPU_PROGRAM_MAP(kramermc_mem)
	MCFG_CPU_IO_MAP(kramermc_io)

	MCFG_DEVICE_ADD("z80pio", Z80PIO, 1500000)
	MCFG_Z80PIO_IN_PA_CB(READ8(kramermc_state, kramermc_port_a_r))
	MCFG_Z80PIO_OUT_PA_CB(WRITE8(kramermc_state, kramermc_port_a_w))
	MCFG_Z80PIO_IN_PB_CB(READ8(kramermc_state, kramermc_port_b_r))

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(64*8, 16*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 16*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(kramermc_state, screen_update_kramermc)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", kramermc )

	MCFG_PALETTE_ADD_MONOCHROME("palette")

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( kramermc )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "io-mon.kmc",   0x0000, 0x0400, CRC(ba230fc8) SHA1(197d4ede31ee8768dd4a17854ee21c468e98b3d6) )
	ROM_LOAD( "debugger.kmc", 0x0400, 0x0400, CRC(5ea3d9e1) SHA1(42e5ced4f965124ae50ec7ac9861d6b668cfab99) )
	ROM_LOAD( "reass.kmc",    0x0800, 0x0400, CRC(7cc8e605) SHA1(3319a96aad710441af30dace906b9725e07ca92c) )
	ROM_LOAD( "basic.kmc",  0x8000, 0x3000, CRC(7531801e) SHA1(61d055495ffcc4a281ef0abc3e299ea95f42544b) )
	ROM_LOAD( "editor.kmc", 0xc000, 0x0400, CRC(2fd4cb84) SHA1(505615a218865aa8becde13848a23e1241a14b96) )
	ROM_LOAD( "ass.kmc",        0xc400, 0x1c00, CRC(9a09422e) SHA1(a578d2cf0ea6eb35dcd13e4107e15187de906097) )
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("chargen.kmc", 0x0000, 0x0800, CRC(1ba52f9f) SHA1(71bbad90dd427d0132c871a4d3848ab3d4d84b8a))
ROM_END

/* Driver */

/*    YEAR  NAME       PARENT  COMPAT  MACHINE    INPUT     CLASS           INIT     COMPANY           FULLNAME       FLAGS */
COMP( 1987, kramermc,  0,      0,      kramermc,  kramermc, kramermc_state, kramermc,"Manfred Kramer", "Kramer MC",   MACHINE_NO_SOUND)