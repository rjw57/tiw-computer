// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Atari Klax hardware

*************************************************************************/
#ifndef MAME_INCLUDES_KLAX_H
#define MAME_INCLUDES_KLAX_H

#pragma once

#include "machine/atarigen.h"
#include "video/atarimo.h"

class klax_state : public atarigen_state
{
public:
	klax_state(const machine_config &mconfig, device_type type, const char *tag)
		: atarigen_state(mconfig, type, tag)
		, m_playfield_tilemap(*this, "playfield")
		, m_mob(*this, "mob")
		, m_p1(*this, "P1")
	{ }

	void klax(machine_config &config);
	void klax2bl(machine_config &config);

protected:
	virtual void machine_reset() override;

	virtual void scanline_update(screen_device &screen, int scanline) override;

	virtual void update_interrupts() override;
	DECLARE_WRITE16_MEMBER(interrupt_ack_w);

	DECLARE_WRITE16_MEMBER(klax_latch_w);

	TILE_GET_INFO_MEMBER(get_playfield_tile_info);
	uint32_t screen_update_klax(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void bootleg_sound_map(address_map &map);
	void klax2bl_map(address_map &map);
	void klax_map(address_map &map);

private:
	required_device<tilemap_device> m_playfield_tilemap;
	required_device<atari_motion_objects_device> m_mob;

	required_ioport m_p1;

	static const atari_motion_objects_config s_mob_config;
};

#endif // MAME_INCLUDES_KLAX_H