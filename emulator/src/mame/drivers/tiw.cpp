// license:BSD-3-Clause
// copyright-holders:Rich Wareham
/*
 * Tiw driver.
 */

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/m6502/m6502.h"
#include "machine/ins8250.h"

#define UART0_TAG       "ns16450_0"
#define RS232A_TAG      "rs232a"

class tiw_state : public driver_device
{
public:
	tiw_state(const machine_config &mconfig, device_type type, const char *tag) :
	          driver_device(mconfig, type, tag),
	          m_maincpu(*this, "maincpu"),
	          m_uart0(*this, UART0_TAG)
	{ }

	DECLARE_DRIVER_INIT(tiw);
	DECLARE_MACHINE_START(tiw);
	DECLARE_MACHINE_RESET(tiw);

	required_device<cpu_device> m_maincpu;
	required_device<ns16450_device> m_uart0;

	void tiw(machine_config &config);
	void tiw_mem(address_map &map);
};

void tiw_state::tiw_mem(address_map &map)
{
	map(0x0000, 0x7FFF).ram().share("ram");
	map(0xD000, 0xD00F).rw(UART0_TAG, FUNC(ns16450_device::ins8250_r), FUNC(ns16450_device::ins8250_w)).umask16(0x00ff);
	map(0xE000, 0xFFFF).rom().region("maincpu", 0);
}

static INPUT_PORTS_START(tiw)
INPUT_PORTS_END

MACHINE_CONFIG_START(tiw_state::tiw)
	MCFG_CPU_ADD("maincpu", M6502, XTAL(4'000'000))
	MCFG_CPU_PROGRAM_MAP(tiw_mem)

	MCFG_DEVICE_ADD( UART0_TAG, NS16450, XTAL(3'686'400) )
	MCFG_INS8250_OUT_DTR_CB(DEVWRITELINE(RS232A_TAG, rs232_port_device, write_dtr))
	MCFG_INS8250_OUT_RTS_CB(DEVWRITELINE(RS232A_TAG, rs232_port_device, write_rts))
	MCFG_INS8250_OUT_TX_CB(DEVWRITELINE(RS232A_TAG, rs232_port_device, write_txd))

	MCFG_RS232_PORT_ADD(RS232A_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, rx_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, dcd_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, cts_w))
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
