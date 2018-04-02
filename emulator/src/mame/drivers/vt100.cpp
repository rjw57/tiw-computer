// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Jonathan Gevaryahu
/***************************************************************************

        DEC VT100 driver by Miodrag Milanovic

        29/04/2009 Preliminary driver.

        TODO: some video attributes are not fully supported yet
        TODO: support for the on-AVO character set roms
        TODO: finish support for the on-cpu board alternate character set rom
        TODO: STP (standard terminal port) bus for VT1XX-AC and VT125

        An enormous amount of useful info can be derived from the VT125 technical manual:
        http://www.bitsavers.org/pdf/dec/terminal/vt100/EK-VT100-TM-003_VT100_Technical_Manual_Jul82.pdf starting on page 6-70, pdf page 316
        And its schematics:
        http://bitsavers.org/pdf/dec/terminal/vt125/MP01053_VT125_Mar82.pdf

*****************************************************************************

        Quick overview of "Set-Up" controls:

        2   Set/Clear Tab (A)       Shift+A   Set Answerback Message (B)
        3   Clear All Tabs (A)      Shift+S   Save Settings
        4   On Line/Local           Shift+R   Recall Settings
        5   Set-Up A/B
        6   Toggle 1/0 (B)
        7   Transmit Speed (B)
        8   Receive Speed (B)
        9   80/132 Columns (A)
        0   Reset Terminal

        If the NVR is not yet initialized, a '2' error will appear when the
        terminal is powered on. This is non-fatal and can ordinarily be
        remedied by entering "Set-Up" and then saving the settings.

****************************************************************************/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/i8085/i8085.h"
#include "cpu/z80/z80.h"
#include "machine/ay31015.h"
#include "machine/com8116.h"
#include "machine/er1400.h"
#include "machine/i8251.h"
#include "machine/input_merger.h"
#include "machine/ins8250.h"
#include "machine/rstbuf.h"
#include "machine/vt100_kbd.h"
#include "video/vtvideo.h"
#include "screen.h"

#include "vt100.lh"


#define RS232_TAG       "rs232"

class vt100_state : public driver_device
{
public:
	vt100_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_crtc(*this, "vt100_video"),
		m_keyboard(*this, "keyboard"),
		m_kbduart(*this, "kbduart"),
		m_pusart(*this, "pusart"),
		m_dbrg(*this, "dbrg"),
		m_nvr(*this, "nvr"),
		m_rstbuf(*this, "rstbuf"),
		m_rs232(*this, RS232_TAG),
		m_printer_uart(*this, "printuart"),
		m_p_ram(*this, "p_ram")
	{
	}

	required_device<cpu_device> m_maincpu;
	required_device<vt100_video_device> m_crtc;
	required_device<vt100_keyboard_device> m_keyboard;
	required_device<ay31015_device> m_kbduart;
	required_device<i8251_device> m_pusart;
	required_device<com8116_device> m_dbrg;
	required_device<er1400_device> m_nvr;
	required_device<rst_pos_buffer_device> m_rstbuf;
	required_device<rs232_port_device> m_rs232;
	optional_device<ins8250_device> m_printer_uart;
	DECLARE_READ8_MEMBER(flags_r);
	DECLARE_WRITE8_MEMBER(baud_rate_w);
	DECLARE_READ8_MEMBER(modem_r);
	DECLARE_WRITE8_MEMBER(nvr_latch_w);
	DECLARE_READ8_MEMBER(printer_r);
	DECLARE_WRITE8_MEMBER(printer_w);
	DECLARE_READ8_MEMBER(video_ram_r);
	DECLARE_WRITE8_MEMBER(uart_clock_w);
	required_shared_ptr<uint8_t> m_p_ram;
	virtual void machine_start() override;
	virtual void machine_reset() override;
	uint32_t screen_update_vt100(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	IRQ_CALLBACK_MEMBER(vt102_irq_callback);
	void vt100(machine_config &config);
	void vt100ac(machine_config &config);
	void vt101(machine_config &config);
	void vt102(machine_config &config);
	void vt180(machine_config &config);
	void vt100_mem(address_map &map);
	void vt100_io(address_map &map);
	void vt102_io(address_map &map);
	void stp_mem(address_map &map);
	void stp_io(address_map &map);
	void vt180_mem(address_map &map);
	void vt180_io(address_map &map);
};




void vt100_state::vt100_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x1fff).rom();  // ROM ( 4 * 2K)
	map(0x2000, 0x2bff).ram().share("p_ram"); // Screen and scratch RAM
	map(0x2c00, 0x2fff).ram();  // AVO Screen RAM
	map(0x3000, 0x3fff).ram();  // AVO Attribute RAM (4 bits wide)
	// 0x4000, 0x7fff is unassigned
	map(0x8000, 0x9fff).rom();  // Program memory expansion ROM (4 * 2K)
	map(0xa000, 0xbfff).rom();  // Program memory expansion ROM (1 * 8K)
	// 0xc000, 0xffff is unassigned
}

void vt100_state::vt180_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x1fff).rom();
	map(0x2000, 0xffff).ram();
}

void vt100_state::vt180_io(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xff);
}

// 0 - XMIT flag H
// 1 - Advance Video L
// 2 - Graphics Flag L
// 3 - Option present H
// 4 - Even field L
// 5 - NVR data H
// 6 - LBA 7 H
// 7 - Keyboard TBMT H
READ8_MEMBER(vt100_state::flags_r)
{
	uint8_t ret = 0;

	ret |= m_pusart->txrdy_r();
	ret |= !m_nvr->data_r() << 5;
	ret |= m_crtc->lba7_r() << 6;
	ret |= m_kbduart->tbmt_r() << 7;
	return ret;
}

WRITE8_MEMBER(vt100_state::baud_rate_w)
{
	m_dbrg->str_w(data & 0x0f);
	m_dbrg->stt_w(data >> 4);
}

READ8_MEMBER(vt100_state::modem_r)
{
	uint8_t ret = 0x0f;

	ret |= m_rs232->cts_r() << 7;
	ret |= m_rs232->si_r() << 6;
	ret |= m_rs232->ri_r() << 5;
	ret |= m_rs232->dcd_r() << 4;

	return ret;
}

WRITE8_MEMBER(vt100_state::nvr_latch_w)
{
	// data inverted due to negative logic
	m_nvr->c3_w(!BIT(data, 3));
	m_nvr->c2_w(!BIT(data, 2));
	m_nvr->c1_w(!BIT(data, 1));

	// C2 is used to disable pullup on data line
	m_nvr->data_w(BIT(data, 2) ? 0 : !BIT(data, 0));

	// SPDS present on pins 11, 19 and 23 of EIA connector
	m_rs232->write_spds(BIT(data, 5));
}

void vt100_state::vt100_io(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xff);
	// 0x00, 0x01 PUSART  (Intel 8251)
	map(0x00, 0x00).rw("pusart", FUNC(i8251_device::data_r), FUNC(i8251_device::data_w));
	map(0x01, 0x01).rw("pusart", FUNC(i8251_device::status_r), FUNC(i8251_device::control_w));
	// 0x02 Baud rate generator
	map(0x02, 0x02).w(this, FUNC(vt100_state::baud_rate_w));
	// 0x22 Modem buffer
	map(0x22, 0x22).r(this, FUNC(vt100_state::modem_r));
	// 0x42 Flags buffer
	map(0x42, 0x42).r(this, FUNC(vt100_state::flags_r));
	// 0x42 Brightness D/A latch
	map(0x42, 0x42).w(m_crtc, FUNC(vt100_video_device::brightness_w));
	// 0x62 NVR latch
	map(0x62, 0x62).w(this, FUNC(vt100_state::nvr_latch_w));
	// 0x82 Keyboard UART data
	map(0x82, 0x82).rw("kbduart", FUNC(ay31015_device::receive), FUNC(ay31015_device::transmit));
	// 0xA2 Video processor DC012
	map(0xa2, 0xa2).w(m_crtc, FUNC(vt100_video_device::dc012_w));
	// 0xC2 Video processor DC011
	map(0xc2, 0xc2).w(m_crtc, FUNC(vt100_video_device::dc011_w));
	// 0xE2 Graphics port
	// AM_RANGE (0xe2, 0xe2)
}

READ8_MEMBER(vt100_state::printer_r)
{
	return m_printer_uart->ins8250_r(space, offset >> 2);
}

WRITE8_MEMBER(vt100_state::printer_w)
{
	m_printer_uart->ins8250_w(space, offset >> 2, data);
}

ADDRESS_MAP_START(vt100_state::vt102_io)
	AM_IMPORT_FROM(vt100_io)
	AM_RANGE(0x03, 0x03) AM_SELECT(0x1c) AM_READ(printer_r)
	AM_RANGE(0x23, 0x23) AM_SELECT(0x1c) AM_WRITE(printer_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vt100 )
INPUT_PORTS_END

uint32_t vt100_state::screen_update_vt100(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_crtc->video_update(bitmap, cliprect);
	return 0;
}


//Interrupts
// in latch A3 - keyboard
//          A4 - receiver
//          A5 - vertical frequency
//          all other set to 1
IRQ_CALLBACK_MEMBER(vt100_state::vt102_irq_callback)
{
	if (irqline == 0)
		return m_rstbuf->inta_cb(device, 0);
	else
		return 0xff;
}

void vt100_state::machine_start()
{
	m_kbduart->write_tsb(0);
	m_kbduart->write_eps(1);
	m_kbduart->write_np(1);
	m_kbduart->write_nb1(1);
	m_kbduart->write_nb2(1);
	m_kbduart->write_cs(1);
	m_kbduart->write_swe(0);

	m_pusart->write_cts(0);

	if (m_printer_uart.found())
	{
		auto *printer_port = subdevice<rs232_port_device>("printer");
		printer_port->write_dtr(0);
		printer_port->write_rts(0);
	}
}

void vt100_state::machine_reset()
{
	nvr_latch_w(machine().dummy_space(), 0, 0);
}

READ8_MEMBER(vt100_state::video_ram_r)
{
	return m_p_ram[offset];
}

WRITE8_MEMBER(vt100_state::uart_clock_w)
{
	m_kbduart->write_tcp(BIT(data, 1));
	m_kbduart->write_rcp(BIT(data, 1));

	if (data == 0 || data == 3)
		m_keyboard->signal_line_w(m_kbduart->so_r());
	else
		m_keyboard->signal_line_w(BIT(data, 0));
}

/* F4 Character Displayer */
static const gfx_layout vt100_charlayout =
{
	8, 16,                  /* 8 x 16 characters */
	256,                    /* 2 x 128 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START( vt100 )
	GFXDECODE_ENTRY( "chargen", 0x0000, vt100_charlayout, 0, 1 )
GFXDECODE_END

MACHINE_CONFIG_START(vt100_state::vt100)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8080, XTAL(24'883'200) / 9)
	MCFG_CPU_PROGRAM_MAP(vt100_mem)
	MCFG_CPU_IO_MAP(vt100_io)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("rstbuf", rst_pos_buffer_device, inta_cb)

	/* video hardware */
	MCFG_SCREEN_ADD_MONOCHROME("screen", RASTER, rgb_t::green())
	MCFG_SCREEN_RAW_PARAMS(XTAL(24'073'400)*2/3, 102*10, 0, 80*10, 262, 0, 25*10)
	//MCFG_SCREEN_RAW_PARAMS(XTAL(24'073'400), 170*9, 0, 132*9, 262, 0, 25*10)
	MCFG_SCREEN_UPDATE_DRIVER(vt100_state, screen_update_vt100)
	MCFG_SCREEN_PALETTE("vt100_video:palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "vt100_video:palette", vt100)
//  MCFG_PALETTE_ADD_MONOCHROME("palette")

	MCFG_DEFAULT_LAYOUT( layout_vt100 )

	MCFG_DEVICE_ADD("vt100_video", VT100_VIDEO, XTAL(24'073'400))
	MCFG_VT_SET_SCREEN("screen")
	MCFG_VT_CHARGEN("chargen")
	MCFG_VT_VIDEO_RAM_CALLBACK(READ8(vt100_state, video_ram_r))
	MCFG_VT_VIDEO_VERT_FREQ_INTR_CALLBACK(DEVWRITELINE("rstbuf", rst_pos_buffer_device, rst4_w))
	MCFG_VT_VIDEO_LBA3_LBA4_CALLBACK(WRITE8(vt100_state, uart_clock_w))
	MCFG_VT_VIDEO_LBA7_CALLBACK(DEVWRITELINE("nvr", er1400_device, clock_w))

	MCFG_DEVICE_ADD("pusart", I8251, XTAL(24'883'200) / 9)
	MCFG_I8251_TXD_HANDLER(DEVWRITELINE(RS232_TAG, rs232_port_device, write_txd))
	MCFG_I8251_DTR_HANDLER(DEVWRITELINE(RS232_TAG, rs232_port_device, write_dtr))
	MCFG_I8251_RTS_HANDLER(DEVWRITELINE(RS232_TAG, rs232_port_device, write_rts))
	MCFG_I8251_RXRDY_HANDLER(DEVWRITELINE("rstbuf", rst_pos_buffer_device, rst2_w))

	MCFG_RS232_PORT_ADD(RS232_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("pusart", i8251_device, write_rxd))
	MCFG_RS232_DSR_HANDLER(DEVWRITELINE("pusart", i8251_device, write_dsr))

	MCFG_DEVICE_ADD("dbrg", COM5016_013, XTAL(24'883'200) / 9) // COM5016T-013 (or WD1943CD-02), 2.7648Mhz Clock
	MCFG_COM8116_FR_HANDLER(DEVWRITELINE("pusart", i8251_device, write_rxc))
	MCFG_COM8116_FT_HANDLER(DEVWRITELINE("pusart", i8251_device, write_txc))

	MCFG_DEVICE_ADD("nvr", ER1400, 0)

	MCFG_DEVICE_ADD("keyboard", VT100_KEYBOARD, 0)
	MCFG_VT100_KEYBOARD_SIGNAL_OUT_CALLBACK(DEVWRITELINE("kbduart", ay31015_device, write_si))

	MCFG_DEVICE_ADD("kbduart", AY31015, 0)
	MCFG_AY31015_WRITE_DAV_CB(DEVWRITELINE("rstbuf", rst_pos_buffer_device, rst1_w))
	MCFG_AY31015_AUTO_RDAV(true)

	MCFG_DEVICE_ADD("rstbuf", RST_POS_BUFFER, 0)
	MCFG_RST_BUFFER_INT_CALLBACK(INPUTLINE("maincpu", 0))
MACHINE_CONFIG_END

ADDRESS_MAP_START(vt100_state::stp_mem)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM AM_REGION("stp", 0)
	AM_RANGE(0x2000, 0x27ff) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START(vt100_state::stp_io)
	AM_RANGE(0x80, 0x80) AM_DEVREADWRITE("stpusart0", i8251_device, data_r, data_w)
	AM_RANGE(0x90, 0x90) AM_DEVREADWRITE("stpusart0", i8251_device, status_r, control_w)
	AM_RANGE(0xa0, 0xa0) AM_DEVREADWRITE("stpusart1", i8251_device, data_r, data_w)
	AM_RANGE(0xb0, 0xb0) AM_DEVREADWRITE("stpusart1", i8251_device, status_r, control_w)
	AM_RANGE(0xc0, 0xc0) AM_DEVREADWRITE("stpusart2", i8251_device, data_r, data_w)
	AM_RANGE(0xd0, 0xd0) AM_DEVREADWRITE("stpusart2", i8251_device, status_r, control_w)
ADDRESS_MAP_END

MACHINE_CONFIG_START(vt100_state::vt100ac)
	vt100(config);
	MCFG_CPU_ADD("stpcpu", I8085A, 4915200)
	MCFG_CPU_PROGRAM_MAP(stp_mem)
	MCFG_CPU_IO_MAP(stp_io)

	MCFG_DEVICE_ADD("stpusart0", I8251, 2457600)
	MCFG_I8251_RXRDY_HANDLER(DEVWRITELINE("stprxint", input_merger_device, in_w<0>))
	MCFG_I8251_TXRDY_HANDLER(DEVWRITELINE("stptxint", input_merger_device, in_w<0>))

	MCFG_DEVICE_ADD("stpusart1", I8251, 2457600)
	MCFG_I8251_RXRDY_HANDLER(DEVWRITELINE("stprxint", input_merger_device, in_w<1>))
	MCFG_I8251_TXRDY_HANDLER(DEVWRITELINE("stptxint", input_merger_device, in_w<1>))

	MCFG_DEVICE_ADD("stpusart2", I8251, 2457600) // for printer?
	MCFG_I8251_RXRDY_HANDLER(DEVWRITELINE("stprxint", input_merger_device, in_w<2>))
	MCFG_I8251_TXRDY_HANDLER(DEVWRITELINE("stptxint", input_merger_device, in_w<2>))

	MCFG_INPUT_MERGER_ANY_HIGH("stptxint")
	MCFG_INPUT_MERGER_OUTPUT_HANDLER(INPUTLINE("stpcpu", I8085_RST55_LINE))

	MCFG_INPUT_MERGER_ANY_HIGH("stprxint")
	MCFG_INPUT_MERGER_OUTPUT_HANDLER(INPUTLINE("stpcpu", I8085_RST65_LINE))

	MCFG_DEVICE_MODIFY("dbrg")
	MCFG_COM8116_FR_HANDLER(DEVWRITELINE("pusart", i8251_device, write_rxc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart0", i8251_device, write_rxc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart1", i8251_device, write_rxc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart2", i8251_device, write_rxc))
	MCFG_COM8116_FT_HANDLER(DEVWRITELINE("pusart", i8251_device, write_txc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart0", i8251_device, write_txc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart1", i8251_device, write_txc))
	MCFG_DEVCB_CHAIN_OUTPUT(DEVWRITELINE("stpusart2", i8251_device, write_txc))
MACHINE_CONFIG_END

MACHINE_CONFIG_START(vt100_state::vt180)
	vt100(config);
	MCFG_CPU_ADD("z80cpu", Z80, XTAL(24'883'200) / 9)
	MCFG_CPU_PROGRAM_MAP(vt180_mem)
	MCFG_CPU_IO_MAP(vt180_io)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(vt100_state::vt101)
	vt100(config);
	MCFG_CPU_REPLACE("maincpu", I8085A, XTAL(24'073'400) / 4)
	MCFG_CPU_PROGRAM_MAP(vt100_mem)
	MCFG_CPU_IO_MAP(vt100_io)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DRIVER(vt100_state, vt102_irq_callback)

	MCFG_DEVICE_MODIFY("pusart")
	MCFG_DEVICE_CLOCK(XTAL(24'073'400) / 8)
	MCFG_I8251_TXRDY_HANDLER(INPUTLINE("maincpu", I8085_RST55_LINE)) // 8085 pin 9, mislabeled RST 7.5 on schematics

	MCFG_DEVICE_REPLACE("dbrg", COM8116_003, XTAL(24'073'400) / 4)
	MCFG_COM8116_FR_HANDLER(DEVWRITELINE("pusart", i8251_device, write_rxc))
	MCFG_COM8116_FT_HANDLER(DEVWRITELINE("pusart", i8251_device, write_txc))

	MCFG_DEVICE_MODIFY("kbduart")
	MCFG_AY31015_WRITE_TBMT_CB(INPUTLINE("maincpu", I8085_RST65_LINE))
MACHINE_CONFIG_END

MACHINE_CONFIG_START(vt100_state::vt102)
	vt101(config);
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_IO_MAP(vt102_io)

	MCFG_DEVICE_ADD("printuart", INS8250, XTAL(24'073'400) / 16)
	MCFG_INS8250_OUT_TX_CB(DEVWRITELINE("printer", rs232_port_device, write_txd))
	MCFG_INS8250_OUT_INT_CB(INPUTLINE("maincpu", I8085_RST75_LINE)) // 8085 pin 7, mislabeled RST 5.5 on schematics

	MCFG_RS232_PORT_ADD("printer", default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("printuart", ins8250_device, rx_w))
	MCFG_RS232_DSR_HANDLER(DEVWRITELINE("printuart", ins8250_device, dsr_w))
MACHINE_CONFIG_END

/* VT1xx models:
 * VT100 - 1978 base model. the 'later' rom is from 1979 or 1980.
 *    The vt100 had a whole series of -XX models branching off of it; the
       ones I know of are described here, as well as anything special about
       them:
 *    VT100-AA - standard model with 120vac cable \__voltage can be switched
 *    VT100-AB - standard model with 240vac cable /  inside any VT100 unit
 *    VT100-W* - word processing series:
 *     VT100-WA/WB - has special LA120 AVO board preinstalled, WP romset?,
        English WP keyboard, no alt charset rom, LA120? 23-069E2 AVO rom.
       (The WA and WB variants are called the '-02' variant on the schematics)
 *    VT100-WC through WZ: foreign language word processing series:
      (The WC through WK variants are called the '-03' variant on the schematics)
 *     VT100-WC/WD - has AVO board preinstalled, WP romset?, French Canadian
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WE/WF - has AVO board preinstalled, WP romset?, French
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WG/WH - has AVO board preinstalled, WP romset?, Dutch
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WJ/WK - has AVO board preinstalled, WP romset?, German
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WY/WZ - has AVO board preinstalled, WP romset?, English
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
       The WP romset supports English, French, Dutch and German languages but
        will only display text properly in the non-English languages if the
        23-094E2 alt charset rom AND the foreign language 23-093E2
        AVO rom are populated.
 *    VT100-NA/NB - ? romset with DECFORM keycaps
 *    VT100 with vt1xx-ac kit - adds serial printer interface (STP)
       pcb, replaces roms with the 095e2/096e2/139e2/140e2 STP set
 * VT101 - 1981 cost reduced unexpandable vt100; Is the same as a stock
   unexpanded vt100. It has no AVO nor the upgrade connector for it, and no
   video input port.) Has its own firmware.
   Shares same pcb with vt102 and vt131, but STP/AVO are unpopulated;
 * VT102 - 1981 cost reduced unexpandable vt100 with built in AVO and STP
   Is the same as a stock vt100 with the AVO and STP expansions installed,
   but all on one pcb. Does NOT support the AVO extended character roms, nor
   the word processing rom set. Has its own firmware.
   Shares same pcb with vt101 and vt131, has STP and AVO populated.
 * VT103 - 1980 base model vt100 with an integrated TU58 tape drive, and an
   LSI-11 backplane, which an LSI-11 cpu card is used in, hence the computer
   is effectively a tiny lsi-11 (pdp-11) built in a vt100 case. uses same roms
   as vt100 for the vt100 portion, and tu58 has its own cpu and rom. It can
   have the normal vt100 romset variant, and also can have the multiple word
   processing variations (which use the same roms as the vt100 ones do).
 * VT104 doesn't exist.
 * VT105 - 1978 vt100 with the WG waveform generator board installed
   (for simple chart-type line-compare-made raster graphics using some built
   in functions), AVO optional; was intended for use on the MINC analog data
   acquisition computer.
 * VT110 - 1978 vt100 with a DPM01 DECDataway serial multiplexer installed
    The DPM01 supposedly has its own processor and roms.
 * vt125 - 1982? base model (stock vt100 firmware plus extra gfx board
   firmware and processor) vt100 with the ReGIS graphical language board
   (aka GPO) installed (almost literally a vk100-on-a-board, but with added
   backwards compatibility mode for vt105/WG, and 2 bits per pixel color),
   AVO optional; Includes a custom 'dumb' STP board.
 * vt131 - 1982 cost reduced version of vt132, no longer has the vt100
   expansion backplane; has the AVO advanced video board built in, as well
   as the parallel port interface board, and supports serial block mode.
   Shares same pcb with vt101 and vt102, has STP and AVO populated.
 * vt132 - 1980? base vt100 with AVO, STP, and its own 23-099e2/23-100e2
   AVO character rom set. Has its own base firmware roms which support block
   serial mode.
 * vt180 - 1980 vt10x (w/vt100 expansion backplane) with a z80 daughterboard
   installed;
   The daughterboard has two roms on it: 23-017e3-00 and 23-021e3-00
   (both are 0x1000 long, 2332 mask roms)
 * vk100 'gigi'- graphical terminal; the vt125 GPO board is a very close derivative;
   relatively little info so far but progress has been made.
   see vk100.c for current driver for this

 * Upgrade kits for vt1xx:
 * VT1xx-AA : p/n 5413206 20ma current loop interface pcb for vt100
 * VT1xx-AB : p/n 5413097 AVO board (AVO roms could be optionally ordered along with
              this board if needed)
 * VT1xx-AC : STP serial printer board (includes a special romset)
 * VT1xx-CA : p/n 5413206? 20ma current loop interface pcb for vt101/vt102/vt131
 * VT1xx-CB or CL: GPO "ReGIS" board vt100->vt125 upgrade kit (p/n 5414275 paddle board and 5414277 gpo board)
 * VT1xx-CE : DECWord Conversion kit
 * VT1xx-FB : Anti-glare kit

 * Info about mask roms and other nasties:
 * A normal 2716 rom has pin 18: /CE; pin 20: /OE; pin 21: VPP (acts as CE2)
 * The vt100 23-031e2/23-061e2, 23-032e2, 23-033e2, and 23-034e2 mask roms
   have the follwing enables:
       23-031e2/23-061e2: pin 18:  CS2; pin 20:  CS1; pin 21:  CS3
       23-032e2:          pin 18: /CS2; pin 20:  CS1; pin 21:  CS3
       23-033e2:          pin 18:  CS2; pin 20:  CS1; pin 21: /CS3
       23-034e2:          pin 18: /CS2; pin 20:  CS1; pin 21: /CS3
       (This is cute because it technically means the roms can be put in the
       4 sockets in ANY ORDER and will still work properly since the cs2 and
       cs3 pins make them self-decode and activate at their proper address)
       (This same cute trick is almost certainly also done with the
       23-180e2, 181e2, 182e2 183e2 romset, as well as the
       23-095e2,096e2,139e2,140e2 set and probably others as well)
 * The vt100/101/102/103/etc 23-018e2-00 character set rom at location e4 is a 24 pin 2316 mask rom with enables as such: pin 18: CS2; pin 20: /CS1; pin 21: /CS3
 * The optional 23-094e2-00 alternate character set rom at location e9 is a 24 pin 2316 mask rom with enables as such: pin 18: /CS2; pin 20: /CS1; pin 21: /CS3
       Supposedly the 23-094e2 rom is meant for vt100-WC or -WF systems, (which are French Canadian and French respectively), implying that it has European language specific accented characters on it. It is probably used in all the -W* systems.
       Pin 21 can be jumpered to +5v for this socket at location e9 by removing jumper w4 and inserting jumper w5, allowing a normal 2716 eprom to be used.
 * The optional AVO character set roms (see below) have: pin 18: /CS2*; pin 20: /CS1; pin 21: CS3 hence they match a normal 2716
   *(this is marked on the image as if it was CS2 but the input is tied to gnd meaning it must be /CS2)

 * The AVO itself can hold up to four roms on it (see http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf
   and http://vt100.net/dec/ek-vt1ac-ug-002.pdf )
   and these roms can depending on jumpers be mapped at 0x8000, OR overlay the main code roms at 0x0000-0x1fff!
   They may even allow banking between the main code roms and the overlay roms, I haven't traced the schematic.
   At least sixteen of these AVO roms were made, and are used as such:
   (based on EK-VT100-TM-003_VT100_Technical_Manual_Jul82.pdf)
 * No roms - normal vt100 system with AVO installed
 * 23-069E2 (location e21) - meant for vt100-wa and -wb 'LA120' 'word processing' systems (the mapping of the rom for this system is different than for the ones below)
 * 23-099E2 (location e21) and 23-100E2 (location e17) - meant for vt132 but only with the OLD vt132 main romset of 095,096,097,098E2
 * 23-093E2 (location e21) - meant for vt100 wc through wz 'foreign language' word processing systems
 * 23-184E2 and 23-185E2 - meant for vt100 with STP printer option board installed, version 1, comes with vt1xx-ac kit
 * 23-186E2 and 23-187E2 - meant for vt100 with STP printer option board installed, version 2, comes with vt1xx-ac kit
 * 23-224E2, 23-225E2, 23-226E2, 23-227E2 - meant for vt132 but only with the NEW vt132 main romset of 180,181,182,183E2
 * 23-236E2, 23-237E2, 23-238E2, 23-239E2 - meant for vt132 but only with the NEW vt132 main romset of 180,181,182,183E2, unknown difference to above (PROM VS MASK ROM? same contents?)
 */

/* ROM definition */
ROM_START( vt100 ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated
// This romset is also used for the vt103, vt105, vt110, vt125, and vt180
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS( "vt100" )
	ROM_SYSTEM_BIOS( 0, "vt100o", "VT100 older roms" )
	ROMX_LOAD( "23-031e2-00.e56", 0x0000, 0x0800, NO_DUMP, ROM_BIOS(1)) // version 1 1978 'earlier rom', dump needed, correct for earlier vt100s
	ROM_SYSTEM_BIOS( 1, "vt100", "VT100 newer roms" )
	ROMX_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15), ROM_BIOS(2)) // version 2 1979 or 1980 'later rom', correct for later vt100s
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL("23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional ?word processing? alternate character set rom
ROM_END

#if 0
ROM_START( vt100wp ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf
// This is the standard vt100 cpu board, with the ?word processing? romset, included in the VT1xx-CE kit?
// the vt103 can also use this rom set (-04 and -05 revs have it by default, -05 rev also has the optional alt charset rom by default)
// NOTE: this is actually the same as the newer VT132 romset; vt132 has different AVO roms as well.
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-180e2-00.e56", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-181e2-00.e52", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-182e2-00.e45", 0x1000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-183e2-00.e40", 0x1800, 0x0800, NO_DUMP)

	ROM_REGION(0x1000, "avo", 0)
	ROM_LOAD( "23-184e2-00.bin", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-185e2-00.bin", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-186e2-00.bin", 0x1000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-187e2-00.bin", 0x1800, 0x0800, NO_DUMP)

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // REQUIRED foreign language alternate character set rom
ROM_END

ROM_START( vt132 ) // This is from anecdotal evidence and vt100.net, as the vt132 schematics are not scanned
// but is pretty much confirmed by page 433 in http://bitsavers.trailing-edge.com/www.computer.museum.uq.edu.au/pdf/EK-VT100-TM-003%20VT100%20Series%20Video%20Terminal%20Technical%20Manual.pdf
// VT100 board with block serial roms, AVO with special roms, STP, custom firmware with block serial mode
// ROMS have Set-Up page C on them
	// OLDER vt132 romset
	ROM_LOAD( "23-095e2-00.e56", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-096e2-00.e52", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-097e2-00.e45", 0x1000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-098e2-00.e40", 0x1800, 0x0800, NO_DUMP)

	// NEWER vt132 (and STP?) romset
	ROM_LOAD( "23-180e2-00.e56", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-181e2-00.e52", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-182e2-00.e45", 0x1000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-183e2-00.e40", 0x1800, 0x0800, NO_DUMP)

	// AVO roms for OLDER romset only
	ROM_REGION(0x1000, "avo", 0)
	ROM_LOAD( "23-099e2-00.e21", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-100e2-00.e17", 0x0800, 0x0800, NO_DUMP)
	// other 2 sockets are empty

	// AVO roms for NEWER romset only
	ROM_LOAD( "23-224e2-00.e21", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-225e2-00.e17", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-226e2-00.e15", 0x1000, 0x0800, NO_DUMP) // loc is a guess
	ROM_LOAD( "23-227e2-00.e13", 0x1800, 0x0800, NO_DUMP) // loc is a guess
	// alt rev of newer avo roms, tech manual implies above are PROMS below are MASK ROMS? same data?
	ROM_LOAD( "23-236e2-00.e21", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-237e2-00.e17", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-238e2-00.e15", 0x1000, 0x0800, NO_DUMP) // loc is a guess
	ROM_LOAD( "23-239e2-00.e13", 0x1800, 0x0800, NO_DUMP) // loc is a guess

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END
#endif

ROM_START( vt100ac ) // This is from the VT180 technical manual at http://www.bitsavers.org/pdf/dec/terminal/vt180/EK-VT18X-TM-001_VT180_Technical_Man_Feb83.pdf
// This is the standard vt100 cpu board, but with the rom set included with the VT1xx-AC kit
// which is only used when the part 54-14260-00 STP 'printer port expansion' card is installed into the terminal board.
// Or as http://bitsavers.trailing-edge.com/www.computer.museum.uq.edu.au/pdf/EK-VT100-TM-003%20VT100%20Series%20Video%20Terminal%20Technical%20Manual.pdf
// on page 433: VT100 WC or WK uses these as well.
// This romset adds the Set-up C page to the setup menu (press keypad 5 twice once you hit set-up)
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-095e2.e40", 0x0000, 0x0800, CRC(6c8acf44) SHA1(b3ef5af920995a40a316c6dc008960c461853bfc)) // Label: "23095E2 // (C)DEC // (M)QQ8227" @E40
	ROM_LOAD( "23-096e2.e45", 0x0800, 0x0800, CRC(77f21473) SHA1(6f10b250777c12cca63ee611735f9f36cc05a7ef)) // Label: "23096E2 // (C)DEC // (M)QQ8227" @E45
	ROM_LOAD( "23-139e2.e52", 0x1000, 0x0800, CRC(c3302ce5) SHA1(a2ab3b9b48b5e850b2d7b22e6f8ef6099599e4b6)) // Label: "AMD // 37108 8232DKP // 23-139E2 // AM9218CPC // (C)DEC_1979" @E52;  // revision 2?; revision 1 is 23-097e2 MAYBE
	ROM_LOAD( "23-140e2.e56", 0x1800, 0x0800, CRC(4bf1ce4e) SHA1(279f47ec9a68c801c3c05005dd782202ac9e51a4)) // Label: "AMD // 37109 8230DHP // 23-140E2 // AM9218CPC // (C)DEC 1979" @E56  // revision 2?; revision 1 is 23-098e2 MAYBE

	ROM_REGION(0x1000, "avo", 0) // all switches on "54-13097-00 // PN2280402L" AVO are open EXCEPT S2-3; does this map at 0xa000-0xcfff (mirrored) in maincpu space?
	// This same set of roms also appears on the "PN1030385J-F" AVO, which has no dipswitches; instead a single jumper? resistor installed in the NDIP20 footprint between E16 and E22, between pins 6 and 15 of the footprint
	//NOTE: for both of these two avo roms, Pin 18 is positive enable CE, Pin 20 is negative enable /CE1, Pin 21 is negative enable /CE2,
	ROM_LOAD( "23-186e2.avo.e21", 0x0000, 0x0800, CRC(1592dec1) SHA1(c4b8fc9fc0514e0cd46ad2de03abe72271ce460b)) // Label: "S 8218 // C69063 // 23186E2" @E21
	ROM_LOAD( "23-187e2.avo.e17", 0x0800, 0x0800, CRC(c6d72a41) SHA1(956f9eb945a250fd05c76100b38c0ba381ab8fde)) // Label: "S 8228 // C69062 // 23187E2" @E17
	// are 184 and 185 an older version of the VT100-AC AVO firmware?

	ROM_REGION(0x2000, "stp", 0) // stp switches 1 and 5 are closed, 2,3,4 open
	ROM_LOAD( "23-029e4.stp.e14", 0x0000, 0x2000, CRC(da55c62b) SHA1(261b02b774d57253d1dedecab8ca0e368c2a96cd)) // Label: "S 8218 // C43020 // 23029E4 (C) DEC // TP02" @E14
	// the rom dump above MIGHT be in the wrong order: it was dumped with A11 to pin 18, A12 to pin 21, A13 to pin 20, but I'm not sure those assignments to pins are correct.
	// At worst the 8 parts of it are in the wrong order.

	ROM_REGION(0x1000, "chargen",0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional ?word processing? alternate character set rom
	ROM_REGION(0x10000, "stpcpu",ROMREGION_ERASEFF)
/* The intelligent STP board: Below is limited information for an ?older version? or prototype of the board
// expansion board for a vt100 with a processor on it and dma, intended to act as a ram/send buffer for the STP printer board.
// It can be populated with two banks of two eproms each, each bank either contains 2k or 4k eproms depending on the w2/w3 and w4/w5 jumpers.
// It also has two proms on the cpu board. I don't know if it is technically necessary to have this board installed if an STP module is installed, but due to the alt stp romset, it probably is.
    ROM_LOAD( "23-003e3-00.e10", 0x0000, 0x1000, NO_DUMP) // "EPROM 0" bank 0
    ROM_LOAD( "23-004e3-00.e4", 0x1000, 0x1000, NO_DUMP) // "EPROM 1" bank 0
    ROM_LOAD( "23-005e3-00.e9", 0x2000, 0x1000, NO_DUMP) // "EPROM 2" bank 1
    ROM_LOAD( "23-006e3-00.e3", 0x3000, 0x1000, NO_DUMP) // "EPROM 3" bank 1
    //ROM_REGION(0x0800, "avo",0)
    //ROM_LOAD( "23-???e2-00.e34", 0x0000, 0x0800, NO_DUMP) // ? second gfx rom?
    ROM_REGION(0x0400, "proms",0)
    ROM_LOAD( "23-312a1-07.e26", 0x0000, 0x0200, NO_DUMP) // "PROM A"; handles 8085 i/o? mapping (usart, timer, dma, comm, etc)
    ROM_LOAD( "23-313a1-07.e15", 0x0200, 0x0200, NO_DUMP) // "PROM B"; handles firmware rom mapping and memory size/page select; bit 0 = ram page, bits 1-3 unused, bits 4-7 select one eprom each
    */
ROM_END

#if 0
ROM_START( vt103 ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt103/MP00731_VT103_Aug80.pdf
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with an
// LSI-11 backplane (instead of a normal VT100 one, hence it cannot use the AVO, WG, GPO, or VT180 Z80 boards) and
// DEC TU58 dual 256k tape drive integrated; It was intended that you would put an LSI-11 cpu card in there, which
// Would talk to the terminal as its input/output device. Several LSI-11 cpu cards were available?
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom

	ROM_REGION(0x0800, "tapecpu", 0) // rom for the 8085 cpu in the integrated serial tu58-xa drive
	ROM_LOAD( "23-089e2.e1", 0x0000, 0x0800, CRC(8614dd4c) SHA1(1b554e6c98bddfc6bc48d81c990deea43cf9df7f)) // Label: "23-089E2 // P8316E - AMD // 35227 8008NPP"

	ROM_REGION(0x80000, "lsi11cpu", 0) // rom for the LSI-11 cpu board
	ROM_LOAD_OPTIONAL( "unknown.bin", 0x00000, 0x80000, NO_DUMP)
ROM_END
#endif

ROM_START( vt105 ) // This is from anecdotal evidence and vt100.net, as the vt105 schematics are not scanned
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// WG waveform generator board factory installed; this makes the terminal act like a vt55 with vt100 terminal capability
// The VT105 was intended for use on the MINC analog data acquisition computer
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END

#if 0
ROM_START( vt110 )
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// DECDataway DPM01 board, which adds 4 or 5 special network-addressable 50ohm? current loop serial lines
// and may add its own processor and ram to control them. see http://bitsavers.org/pdf/dec/terminal/EK-VT110_UG-001_Dec78.pdf
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
//DECDataway board roms go here!
ROM_END

ROM_START( vt125 ) // This is from bitsavers and vt100.net, as the vt125 schematics are not scanned
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// special "GPO" ReGIS cpu+ram card 54-14277 installed which provides a framebuffer, text rotation, custom ram fonts, and many other features.
// Comes with a custom 'dumb' STP card 54-14275 as well.
// VT125 upgrade kit (upgrade from vt100 or vt105) was called VT1xx-CB or CL
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom

	// "GPO" aka vt125 "mono board" roms and proms
	ROM_REGION(0x10000, "monocpu", ROMREGION_ERASEFF) // roms for the 8085 subcpu
	ROM_LOAD( "23-043e4-00.e22", 0x0000, 0x2000, NO_DUMP) // 2364/MK36xxx mask rom
	ROM_LOAD( "23-044e4-00.e23", 0x2000, 0x2000, NO_DUMP) // 2364/MK36xxx mask rom
	ROM_LOAD( "23-045e4-00.e24", 0x4000, 0x2000, NO_DUMP) // 2364/MK36xxx mask rom
	// E25 socket is empty

	ROM_REGION(0x100, "dir", ROMREGION_ERASEFF ) // vt125 direction prom, same as on vk100, 82s135 equiv
	ROM_LOAD( "23-059b1.e41", 0x0000, 0x0100, CRC(4b63857a) SHA1(3217247d983521f0b0499b5c4ef6b5de9844c465))

	ROM_REGION(0x100, "trans", ROMREGION_ERASEFF ) // vt125 x translate prom, same as on vk100, 82s135 equiv
	ROM_LOAD( "23-060b1.e60", 0x0000, 0x0100, CRC(198317fc) SHA1(00e97104952b3fbe03a4f18d800d608b837d10ae))

	ROM_REGION(0x500, "proms", ROMREGION_ERASEFF) // vt125 mono board proms
	ROM_LOAD( "23-067b1.e135", 0x0000, 0x0100, NO_DUMP) // 82s135, waitstate prom
	ROM_LOAD( "23-068b1.e64", 0x0100, 0x0100, NO_DUMP) // 82s135, sync_a prom
	ROM_LOAD( "23-069b1.e66", 0x0200, 0x0100, NO_DUMP) // 82s135, sync_b prom
	ROM_LOAD( "23-070b1.e71", 0x0300, 0x0100, NO_DUMP) // 82s135, vector prom
	ROM_LOAD( "23-582a2.e93", 0x0400, 0x0100, NO_DUMP) // 82s129, ras/erase prom
ROM_END
#endif

ROM_START( vt101 ) // p/n 5414185-01 'unupgradable/low cost' vt101/vt102/vt131 mainboard
// does not have integrated STP or AVO populated
// 8085 based instead of I8080
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-028e4-00.e71", 0x0000, 0x2000, CRC(fccce02c) SHA1(f3e3e93a857443685b816cab4fb52e34c0bc72b1)) // rom is unique to vt101; "CN55004N 8232 // DEC TP03 // 23-028E4-00" 24-pin mask rom (mc68764 pinout)
	// E69 socket is empty/unpopulated on vt101
	// E67 socket is empty/unpopulated on vt101
	// DIP40 at E74 in the lower right corner of MB in vt101 (WD8250 UART) is absent

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e3", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e4", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END


ROM_START( vt102 ) // p/n 5414185-01 'unupgradable/low cost' vt101/vt102/vt131 mainboard
// has integrated STP and AVO both populated
// ROMS have the set up page C in them
// 8085 based instead of I8080
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS( "vt102" )
	ROM_SYSTEM_BIOS( 0, "vt102o", "VT102 older roms" )
	ROMX_LOAD( "23-042e4-00.e71", 0x0000, 0x2000, CRC(e8aa006c) SHA1(8ac2a84a8d2a9fa0c6cd583ae35e4c21f863b45b), ROM_BIOS(1)) // shared with vt131
	ROMX_LOAD( "23-041e4-00.e69", 0x8000, 0x2000, CRC(b11d331e) SHA1(8b0f885c7e032d1d709e3913d279d6950bbd4b6a), ROM_BIOS(1)) // shared with vt131
	ROM_SYSTEM_BIOS( 1, "vt102", "VT102 newer roms" )
	ROMX_LOAD( "23-226e4-00.e71", 0x0000, 0x2000, CRC(85c9279a) SHA1(3283d27e9c45d9e384227a7e6e98ee8d54b92bcb), ROM_BIOS(2)) // shared with vt131
	ROMX_LOAD( "23-225e4-00.e69", 0x8000, 0x2000, CRC(3567c760) SHA1(672473162e9c92cd237e4dbf92c2700a31c5374b), ROM_BIOS(2)) // shared with vt131
	//e67 socket is empty on vt102 but populated on vt131 below

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e3", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e4", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END

ROM_START( vt131 ) // p/n 5414185-01 'unupgradable/low cost' vt101/vt131 mainboard with vt132-style block serial mode
// has integrated STP and AVO both populated
// ROMS have the set up page C in them
// 8085 based instead of I8080
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS( "vt131" )
	ROM_SYSTEM_BIOS( 0, "vt131o", "VT131 older roms" )
	ROMX_LOAD( "23-042e4-00.e71", 0x0000, 0x2000, CRC(e8aa006c) SHA1(8ac2a84a8d2a9fa0c6cd583ae35e4c21f863b45b), ROM_BIOS(1)) // shared with vt102
	ROMX_LOAD( "23-041e4-00.e69", 0x8000, 0x2000, CRC(b11d331e) SHA1(8b0f885c7e032d1d709e3913d279d6950bbd4b6a), ROM_BIOS(1)) // shared with vt102
	ROM_SYSTEM_BIOS( 1, "vt131", "VT131 newer roms" )
	ROMX_LOAD( "23-226e4-00.e71", 0x0000, 0x2000, CRC(85c9279a) SHA1(3283d27e9c45d9e384227a7e6e98ee8d54b92bcb), ROM_BIOS(2)) // shared with vt102
	ROMX_LOAD( "23-225e4-00.e69", 0x8000, 0x2000, CRC(3567c760) SHA1(672473162e9c92cd237e4dbf92c2700a31c5374b), ROM_BIOS(2)) // shared with vt102
	ROM_LOAD( "23-280e2-00.e67", 0xA000, 0x0800, CRC(71b4172e) SHA1(5a82c7dc313bb92b9829eb8350840e072825a797)) // called "VT131 ROM" in the vt101 quick reference guide; pins 20, 18 and 21 are /CE /CE2 and /CE3 on this mask rom

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e3", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL( "23-094e2-00.e4", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END

ROM_START( vt180 )
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// Z80 daughterboard added to the expansion slot, and replacing the STP adapter (STP roms are replaced with the normal set)
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom

	ROM_REGION(0x10000, "z80cpu", 0) // z80 daughterboard
	ROM_LOAD( "23-021e3-00.bin", 0x0000, 0x1000, CRC(a2a575d2) SHA1(47a2c40aaec89e8476240f25515d75ab157f2911))
	ROM_LOAD( "23-017e3-00.bin", 0x1000, 0x1000, CRC(4bdd2398) SHA1(84f288def6c143a2d2ed9dedf947c862c66bb18e))
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY                     FULLNAME       FLAGS */
COMP( 1978, vt100,    0,      0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT100",MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
//COMP( 1978, vt100wp,  vt100,  0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT100-Wx", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1979, vt100ac,  vt100,  0,       vt100ac,   vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT100 w/VT1xx-AC STP", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1981, vt101,    vt102,  0,       vt101,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT101", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1981, vt102,    0,      0,       vt102,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT102", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
//COMP( 1979, vt103,    vt100,  0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT103", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1978, vt105,    vt100,  0,       vt100,     vt100, vt100_state,   0,   "Digital Equipment Corporation", "VT105", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
//COMP( 1978, vt110,    vt100,  0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT110", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
//COMP( 1981, vt125,    vt100,  0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT125", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1981, vt131,    vt102,  0,       vt102,     vt100, vt100_state,   0,   "Digital Equipment Corporation", "VT131", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
//COMP( 1979, vt132,    vt100,  0,       vt100,     vt100, vt100_state,   0,  "Digital Equipment Corporation", "VT132", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)
COMP( 1983, vt180,    vt100,  0,       vt180,     vt100, vt100_state,   0,   "Digital Equipment Corporation", "VT180", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS)