// license:BSD-3-Clause
// copyright-holders:Rich Wareham
/*
 * Tiw driver.
 */

#include "emu.h"

#include "cpu/m6502/m6502.h"

class tiw_state : public driver_device
{
public:
	tiw_state(const machine_config &mconfig, device_type type, const char *tag) :
	           driver_device(mconfig, type, tag),
	           m_maincpu(*this, "maincpu")
	{ }

	DECLARE_DRIVER_INIT(tiw);
	DECLARE_MACHINE_START(tiw);
	DECLARE_MACHINE_RESET(tiw);

	required_device<cpu_device> m_maincpu;

	void tiw(machine_config &config);
	void tiw_mem(address_map &map);
};

ADDRESS_MAP_START(tiw_state::tiw_mem)
	AM_RANGE(0x0000, 0x7FFF) AM_RAM
	AM_RANGE(0xE000, 0xFFFF) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

static INPUT_PORTS_START(tiw)
INPUT_PORTS_END

MACHINE_CONFIG_START(tiw_state::tiw)
	MCFG_CPU_ADD("maincpu", M6502, XTAL(4'000'000))
	MCFG_CPU_PROGRAM_MAP(tiw_mem)
MACHINE_CONFIG_END

ROM_START(tiw)
	ROM_REGION(0x2000, "maincpu", 0)
	ROM_LOAD("rom.bin", 0x0000, 0x2000, CRC(e527d758))
ROM_END

DRIVER_INIT_MEMBER(tiw_state, tiw)
{
}

MACHINE_START_MEMBER(tiw_state, tiw)
{
}

MACHINE_RESET_MEMBER(tiw_state, tiw)
{
}

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    CLASS         INIT    COMPANY                FULLNAME                FLAGS */
COMP(2018,  tiw,    0,      0,       tiw,       tiw,     tiw_state,    0,      "Rich Wareham",       "Tiw homebrew computer", MACHINE_TYPE_COMPUTER | MACHINE_NO_SOUND)
