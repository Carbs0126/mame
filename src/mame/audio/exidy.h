// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef MAME_AUDIO_EXIDY_H
#define MAME_AUDIO_EXIDY_H

#pragma once

#include "machine/6532riot.h"
#include "machine/6821pia.h"
#include "sound/hc55516.h"
#include "sound/tms5220.h"


class exidy_sound_device : public device_t,
									public device_sound_interface
{
	/* 6840 variables */
	struct sh6840_timer_channel
	{
		uint8_t   cr;
		uint8_t   state;
		uint8_t   leftovers;
		uint16_t  timer;
		uint32_t  clocks;
		union
		{
#ifdef LSB_FIRST
			struct { uint8_t l, h; } b;
#else
			struct { uint8_t h, l; } b;
#endif
			uint16_t w;
		} counter;
	};

public:
	exidy_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	~exidy_sound_device() {}

	DECLARE_READ8_MEMBER(sh6840_r);
	DECLARE_WRITE8_MEMBER(sh6840_w);
	DECLARE_WRITE8_MEMBER(sfxctrl_w);

protected:
	exidy_sound_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	void common_sh_start();
	void common_sh_reset();

	void sh6840_register_state_globals();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) override;
	virtual stream_sample_t generate_music_sample();

	static inline void sh6840_apply_clock(sh6840_timer_channel *t, int clocks);

	/* sound streaming variables */
	sound_stream *m_stream;
	double m_freq_to_step;

private:
	// internal state
	sh6840_timer_channel m_sh6840_timer[3];
	int16_t m_sh6840_volume[3];
	uint8_t m_sh6840_MSB_latch;
	uint8_t m_sh6840_LSB_latch;
	uint8_t m_sh6840_LFSR_oldxor;
	uint32_t m_sh6840_LFSR_0;
	uint32_t m_sh6840_LFSR_1;
	uint32_t m_sh6840_LFSR_2;
	uint32_t m_sh6840_LFSR_3;
	uint32_t m_sh6840_clocks_per_sample;
	uint32_t m_sh6840_clock_count;

	uint8_t m_sfxctrl;

	inline int sh6840_update_noise(int clocks);
};

DECLARE_DEVICE_TYPE(EXIDY, exidy_sound_device)

class exidy_sh8253_sound_device : public exidy_sound_device
{
	struct sh8253_timer_channel
	{
		uint8_t   clstate;
		uint8_t   enable;
		uint16_t  count;
		uint32_t  step;
		uint32_t  fraction;
	};

protected:
	exidy_sh8253_sound_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual stream_sample_t generate_music_sample() override;

public:
	DECLARE_WRITE8_MEMBER(r6532_porta_w);
	DECLARE_READ8_MEMBER(r6532_porta_r);
	DECLARE_WRITE8_MEMBER(r6532_portb_w);
	DECLARE_READ8_MEMBER(r6532_portb_r);

	DECLARE_WRITE8_MEMBER(sh8253_w);
	DECLARE_READ8_MEMBER(sh8253_r);

protected:
	void sh8253_register_state_globals();

	/* 8253 variables */
	sh8253_timer_channel m_sh8253_timer[3];

	/* 6532 variables */
	required_device<riot6532_device> m_riot;

	/* 5220/CVSD variables */
	optional_device<hc55516_device> m_cvsd;
	optional_device<cpu_device> m_cvsdcpu;
	optional_device<tms5220_device> m_tms;
	required_device<pia6821_device> m_pia1;
};

class venture_sound_device : public exidy_sh8253_sound_device
{
public:
	venture_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	DECLARE_WRITE8_MEMBER( mtrap_voiceio_w );
	DECLARE_READ8_MEMBER( mtrap_voiceio_r );

	DECLARE_WRITE8_MEMBER( filter_w );

	void venture_audio(machine_config &config);
protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) override;
};

DECLARE_DEVICE_TYPE(EXIDY_VENTURE, venture_sound_device)

class victory_sound_device : public exidy_sh8253_sound_device
{
public:
	victory_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	DECLARE_READ8_MEMBER( response_r );
	DECLARE_READ8_MEMBER( status_r );
	DECLARE_WRITE8_MEMBER( command_w );
	DECLARE_WRITE_LINE_MEMBER( irq_clear_w );
	DECLARE_WRITE_LINE_MEMBER( main_ack_w );

	void victory_audio(machine_config &config);
protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) override;

private:
	// internal state
	uint8_t m_victory_sound_response_ack_clk; /* 7474 @ F4 */

	TIMER_CALLBACK_MEMBER( delayed_command_w );

	int m_pia1_ca1;
	int m_pia1_cb1;
};

DECLARE_DEVICE_TYPE(EXIDY_VICTORY, victory_sound_device)

#endif // MAME_AUDIO_EXIDY_H
