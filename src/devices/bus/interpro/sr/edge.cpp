// license:BSD-3-Clause
// copyright-holders:Patrick Mackinlay

/*
 * An emulation of Extensible Display Geometry Engine (EDGE) graphics, for
 * Intergraph InterPro systems.
 *
 * TODO
 *   - this is a WIP checkpoint, very little is working
 *   - dumb framebuffer is working in 8bpp mode
 *   - missing dumps for EDGE-2 EPROMs
 *
 * List of EDGE graphics cards:
 *
 *    004   EDGE-2 Processor f/2 2Mp-FB's
 *    009   EDGE II Frame Buffer for ImageStation
 *    030   EDGE-2 Processor f/1 or 2 1Mp-FB's
 *    031   EDGE-2 Processor f/1 2Mp-FB
 *    093   EDGE-2 Plus Processor f/1 or 2 1Mp-FB's
 *    094   EDGE-2 Plus Processor f/1 2Mp-FB
 *    095   EDGE-2 Plus Processor f/2 2Mp-FB's
 *    096   EDGE-2 Plus Frame Buffer f/1Mp Monitor
 *    828   EDGE-1 Graphics f/1 1Mp Monitor (55K/60)
 *    849   EDGE-1 Graphics f/1 2Mp Monitor (55K/60)
 *    896   EDGE-2/Plus Frame Buffer f/2Mp Monitor (V-60)
 *    897   EDGE-2 Processor f/1 2Mp-FB
 *    904   EDGE-1 Graphics f/2 1Mp Monitors (55K/60)
 *    965   EDGE-1 Graphics f/1 1Mp Monitor (66K/72)
 *    966   EDGE-1 Graphics f/2 1Mp Monitors (66K/72)
 *    A63   EDGE-2 Frame Buffer f/1Mp Monitor (55K/60)
 *    B15   EDGE-1 Graphics f/1 1Mp Monitor (55K/60) -T
 *    B16   EDGE-1 Graphics f/2 1Mp Monitors (55K/60) -T
 *
 * EDGE-1 (PCB828)
 *
 *   Ref   Part                      Function
 *   U126  TMS320C30GBL              DSP
 *   U173  ST D9026                  SRX bus interface?
 *         MKJF03WG-00 C
 *         CICD7140A
 *   U214  Z8530H-6JC                Keyboard/tablet serial controller
 *   U212  DP8510V                   Bitblt unit
 *   U229  DP8510V                   Bitblt unit
 *   U221  Bt 458KG110               RAMDAC
 *   U247  80.0208 MHz crystal       Pixel clock source
 *   Y1    30.0 MHz crystal          DSP clock source
 *   Y2    4.9152 MHz crystal        SCC clock source
 *         TC55417P-25H              16Kx4 static RAM (16 parts, total 128KiB)
 *         Am7202-25RC               1024x9 FIFO (4 parts)
 *         TC524258AZ-10             256Kx4 VRAM (20 parts, total 2560KiB)
 *
 * EDGE-2 Plus Processor (SMT094)
 *
 *   Ref   Part                      Function
 *   U45   Zilog Z0853006VSC SCC     Serial controller
 *   U49   Intergraph CICD87307
 *         202100
 *   U94   NS SCX6B64ABM/NU6
 *   U107  VLSI CICD93708
 *         VGT8203C4588
 *   U132  VLSI CICD93809
 *         8207C4587
 *   U133  TMS320C30GBL              DSP
 *   U134  ST ZDD0J131               SRX bus interface?
 *         MKJF03WG-00 C
 *         CICD7140A
 *         Z104463BA
 *   U191  NS SCX6B86ABN/NU6
 *   U201  TMX320C30GEL40            DSP
 *   U206? MPRGP020A 31 MAR 92       EPROM
 *   U207? MPRGP010A 04 01 92        EPROM
 *   U222  MPRGP060B 16 MAR 92       EPROM
 *   U239  MPRGP050B APR 02 92       EPROM
 *   U306  MPRGP080B APR 02 92       EPROM
 *   U310  MPRGP070B APR 02 92       EPROM
 *   U313  MPRGP040B APR 02 92       EPROM
 *   U303  TMX320C30GEL40            DSP
 *   Y1    4.915200MHz crystal
 *   Y2    66.666700MHz crystal
 *   Y3    33.3333MHz crystal
 *   Y4?   40.0MHz crystal
 *
 * EDGE-2 Plus Frame Buffer (PCB896)
 *
 *   Ref   Part                      Function
 *   U11   CICD87408 TC110G14AY
 *   U336  Brooktree PS045701-165    Single-channel RAMDAC (blue)
 *   U337  Brooktree PS045701-165    Single-channel RAMDAC (green)
 *   U338  Brooktree PS045701-165    Single-channel RAMDAC (red)
 *   U340  Brooktree Bt439KC         Clock generator and synchronizer
 *   U360  164.609300MHz crystal     Pixel clock
 *
 *         MCM6294P20                16Kx4 Bit Synchronous Static RAM (total 256KiB)
 *   U233-240                        group 1, 8 parts = 16Kx32
 *   U300-323                        group 2, 24 parts = 16Kx32x3
 *
 * Processor board idprom feature byte 0 contains various flags:
 *
 *   if ((id->i_feature[0] & 0x04) == 0)
 *     EDGctrl.c_flag |= EDGEII_PLUS;
 *
 * On the frame buffer:
 *   if (id->i_feature[0] & 3 == 0) EDGE_M2_1MPIX else EDGE_M2_2MPIX
 */

/*
 * Work in progress notes (EDGE-2 Plus)
 *
 * processor board: 0x8f008000
 *        000 mouse/interrupt
 *
 *        008 mouse X
 *        00c mouse Y
 *        010 scc chan B control (tablet)
 *        014 scc chan A control (keyboard)
 *        018 scc chan B data (tablet)
 *        01c scc chan A data (keyboard)
 *
 *        SRX bus registers
 *        100 system control
 *        104 system status
 *        108 SRX input fifo (W/O)
 *        10c SRX kernel
 *        110 SRX mapping (W/O)
 *        114 SRX attention
 *
 *        130 ififo low water mark (W/O)
 *        134 ififo high water mark (W/O)
 *
 *       ff80 idprom
 *
 * SRX region 4
 *   SRreg4 = 0x90000000  // Emerald uses 0xbf000000-0xffffffff
 *   SRreg4 + 0x02028298 - virtual contrast register
 *   SRreg4 + 0x02028088 - base address/fb enable?
 *   SRreg4 += 0x04000000 // 64M - 32M memory, 32M regs?

 * 0x90000000 256kB static ram (end address 0x9003ffff)
 * video ram?
 *
 * 0x91000000-0x91ffffff == 16 megabytes, probably video ram, where is the remaining 2M?
 *
 * 0x92028000
 *        084 <- 0
 *        088 base address/fb enable <- 0, 8, 9, a, b (typically reads from 92028300 after writing)?
 *            bottom 2 bits are board select?
 *        098 <- 0x00ffffff
 * 0x92028100
 * 0x92028200 - framebuffer board
 *        200 idprom
 *        280 RAMDAC addr reg
 *        284 RAMDAC CPAL reg - colour palette
 *        288 RAMDAC ctrl reg
 *        28c RAMDAC OPAL reg - overlay palette
 *        290 lookup table select reg
 *        294 pixel tag LUT context reg
 *        298 contrast reg
 *
 * 0x92028300 - ?
 *        300 = 1, r/w
 *        304 read in a loop waiting for value & 0x1000 == 0
 *        354 = ff
 *
 * 0x92028400 - processor board (P-bus?)
 *        400 P-bus status (maybe shifted attention bits?), 0x1000 == KREG IN FULL
 *        404 pipeline ctrl/sts
 *        408 kernel
 *        40c IFIFO (integer)
 *        500 SRX master data
 *        504 SRX master addr
 *        508 SRX master attn/ctrl
 *        50c IFIFO (floating point)
 *
 * 0x92030000 - unknown at least 256 dwords
 *
 * RE - render engine
 * ILUT - inverse LUT?
 * ISQLUT - inverse square root LUT
 *
 * FIFO 2, FIFO 3, render FIFO
 * P-bus, S-bus, R-bus
 * static RAM
 *
 * P-bus == TMS320C30 peripheral/primary bus?
 * C30 has primary (24 bit address) and expansion (13 bit address) bus, both 32 bit data
 *
 * peripheral bus is internal (serial and timers)
 *
 * sram test at 5a76ec, sub_5a765c, sub_5a76b0(ram size in dwords)
 * video ram test at sub_5a7ad0
 * address 5a33de, 5a2dbc
 */

#include "emu.h"
#include "edge.h"

#define LOG_GENERAL (1U << 0)

#define VERBOSE (LOG_GENERAL)

#include "logmacro.h"

DEFINE_DEVICE_TYPE(MPCB828, mpcb828_device, "mpcb828", "EDGE-1 Graphics f/1 1Mp Monitor (55K/60)")
DEFINE_DEVICE_TYPE(MPCB849, mpcb849_device, "mpcb849", "EDGE-1 Graphics f/1 2Mp Monitor (55K/60)")
DEFINE_DEVICE_TYPE(MPCB030, mpcb030_device, "mpcb030", "EDGE-2 Processor f/1 or 2 1Mp-FB's")
DEFINE_DEVICE_TYPE(MPCBA63, mpcba63_device, "mpcba63", "EDGE-2 Frame Buffer f/1Mp Monitor (55K/60)")
DEFINE_DEVICE_TYPE(MSMT094, msmt094_device, "msmt094", "EDGE-2 Plus Processor f/1 2Mp-FB")
DEFINE_DEVICE_TYPE(MPCB896, mpcb896_device, "mpcb896", "EDGE-2/Plus Frame Buffer f/2Mp Monitor (V-60)")

void edge1_device_base::map(address_map &map)
{
	srx_card_device_base::map(map);

	/*
	 * ODT reports
	 * 000 user 1 mouse/interrupt
	 * 008 user 1 mouse X counter
	 * 00c user 1 mouse Y counter
	 * 020 user 2 mouse/interrupt
	 * 028 user 2 mouse X counter
	 * 02c user 2 mouse Y counter
	 * 100 system control
	 * 104 system status
	 * 108 input FIFO (W/O)
	 * 10c kernel
	 * 110 mapping (W/O)
	 * 114 attention
	 */
	map(0x000, 0x003).rw(this, FUNC(edge1_device_base::reg0_r), FUNC(edge1_device_base::reg0_w));

	map(0x010, 0x01f).rw("scc", FUNC(z80scc_device::cd_ab_r), FUNC(z80scc_device::cd_ab_w)).umask32(0x000000ff);

	map(0x100, 0x103).rw(this, FUNC(edge1_device_base::control_r), FUNC(edge1_device_base::control_w));
	map(0x104, 0x107).rw(this, FUNC(edge1_device_base::status_r), FUNC(edge1_device_base::status_w));
	map(0x108, 0x10b).rw(this, FUNC(edge1_device_base::fifo_r), FUNC(edge1_device_base::fifo_w));
	map(0x10c, 0x10f).rw(this, FUNC(edge1_device_base::kernel_r), FUNC(edge1_device_base::kernel_w));

	map(0x114, 0x117).rw(this, FUNC(edge1_device_base::attention_r), FUNC(edge1_device_base::attention_w));

	map(0x130, 0x133).w(this, FUNC(edge1_device_base::ififo_lwm_w));
	map(0x134, 0x137).w(this, FUNC(edge1_device_base::ififo_hwm_w));
}

void edge1_device_base::map_dynamic(address_map &map)
{
	// TODO: map using lambdas until mixed-size submaps work
	map(0x00000000, 0x0001ffff).lrw8("sram",
		[this](address_space &space, offs_t offset, u8 mem_mask) { return m_sram->read(offset); },
		[this](address_space &space, offs_t offset, u8 data, u8 mem_mask) { m_sram->write(offset, data); });

	map(0x01000000, 0x013fffff).lrw8("vram",
		[this](address_space &space, offs_t offset, u8 mem_mask) { return m_vram->read((offset >> 2) | (offset & 0x3)); },
		[this](address_space &space, offs_t offset, u8 data, u8 mem_mask) { m_vram->write((offset >> 2) | (offset & 0x3), data); });

	//map(0x02028200, 0x0202827f).lr32("idprom",
	//	[this](address_space &space, offs_t offset, u8 mem_mask) { return memregion("idprom")->as_u32(offset); });

	map(0x02410000, 0x0241000f).m("ramdac", FUNC(bt458_device::map)).umask32(0x000000ff);

	// 0x02400004
	// 0x02400088
	// 0x0240008c
	// 0x02400008
	// 0x02400098 - contrast

	// 0x02410014
	// 0x02410018 - ramdac/screen/buffer select?
	// 0x02410040 - reg0?
}

void edge2_processor_device_base::map(address_map &map)
{
	srx_card_device_base::map(map);

	map(0x100, 0x103).nopr(); // control
	map(0x104, 0x107).nopr(); // status/attention
	map(0x108, 0x10b).nopr(); // fifo
	map(0x10c, 0x10f).nopr(); // kreg

	map(0x114, 0x117).nopr(); // status/interrupt

	map(0x130, 0x133).nopr(); // fifo low water mark
	map(0x134, 0x137).nopr(); // fifo high water mark

	// SRreg4 = 0x90000000
	// SRreg4 + 0x02028298 - virtual contrast register
	// SRreg4 + 0x02028088 - base address/fb enable?
}

void edge2plus_processor_device_base::map(address_map &map)
{
	srx_card_device_base::map(map);

	map(0x000, 0x003).rw(this, FUNC(edge2plus_processor_device_base::reg0_r), FUNC(edge2plus_processor_device_base::reg0_w));

	map(0x010, 0x01f).rw("scc", FUNC(z80scc_device::cd_ab_r), FUNC(z80scc_device::cd_ab_w)).umask32(0x000000ff);

	map(0x100, 0x103).rw(this, FUNC(edge2plus_processor_device_base::control_r), FUNC(edge2plus_processor_device_base::control_w));
	map(0x104, 0x107).rw(this, FUNC(edge2plus_processor_device_base::status_r), FUNC(edge2plus_processor_device_base::status_w));

	map(0x10c, 0x10f).rw(this, FUNC(edge2plus_processor_device_base::kernel_r), FUNC(edge2plus_processor_device_base::kernel_w));
	map(0x110, 0x113).w(this, FUNC(edge2plus_processor_device_base::mapping_w));
	map(0x114, 0x117).rw(this, FUNC(edge2plus_processor_device_base::attention_r), FUNC(edge2plus_processor_device_base::attention_w));

	map(0x130, 0x133).w(this, FUNC(edge2plus_processor_device_base::ififo_lwm_w));
	map(0x134, 0x137).w(this, FUNC(edge2plus_processor_device_base::ififo_hwm_w));

	// 0x300 fifo


	// scnxdiag only on II+
}

// NOTE: at boot, boards 0-3 are selected, and the idprom mirror probed - each valid idprom
// is identified as being an additional board.
void edge2plus_framebuffer_device_base::map_dynamic(address_map &map)
{
	// TODO: map using lambdas until mixed-size submaps work

	map(0x00000000, 0x0003ffff).lrw8("sram",
	[this](address_space &space, offs_t offset, u8 mem_mask) { return m_sram->read(offset); },
		[this](address_space &space, offs_t offset, u8 data, u8 mem_mask) { m_sram->write(offset, data); });

	map(0x01000000, 0x01ffffff).lrw8("vram", 
		[this](address_space &space, offs_t offset, u8 mem_mask) { return m_vram->read(offset); },
		[this](address_space &space, offs_t offset, u8 data, u8 mem_mask) { m_vram->write(offset, data); });

	map(0x02028088, 0x0202808b).w(this, FUNC(edge2plus_framebuffer_device_base::select_w));

	map(0x02028200, 0x0202827f).lr32("idprom",
		[this](address_space &space, offs_t offset, u8 mem_mask) { return m_select == 0 ? memregion("idprom")->as_u32(offset) : space.unmap(); });

	map(0x02028290, 0x02028293).w(this, FUNC(edge2plus_framebuffer_device_base::lut_select_w));
	map(0x02028300, 0x02028303).w(this, FUNC(edge2plus_framebuffer_device_base::unk_300_w));
	map(0x02028304, 0x02028307).r(this, FUNC(edge2plus_framebuffer_device_base::unk_304_r));
}

void edge2_framebuffer_device_base::map(address_map &map)
{
	srx_card_device_base::map(map);
}

void edge2plus_framebuffer_device_base::map(address_map &map)
{
	srx_card_device_base::map(map);
}

ROM_START(mpcb828)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("mpcb828d.bin", 0x0, 0x20, CRC(997c09f1) SHA1(5378e33ef62309bc0c8cbe9ccbd87fd871bdc195))
ROM_END

ROM_START(mpcb849)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("mpcb849a.bin", 0x0, 0x20, NO_DUMP)
ROM_END

ROM_START(mpcb030)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("mpcb030a.bin", 0x0, 0x20, NO_DUMP)
ROM_END

ROM_START(mpcba63)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("mpcba63b.bin", 0x0, 0x20, NO_DUMP)
ROM_END

ROM_START(msmt094)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("msmt0940.bin", 0x0, 0x20, CRC(55493b9e) SHA1(6e0668f7e85e1bb5b2ecc6e5a5ab53e5281f22e9))
ROM_END

ROM_START(mpcb896)
	ROM_REGION(0x80, "idprom", 0)
	ROM_LOAD32_BYTE("mpcb896b.bin", 0x0, 0x20, CRC(c8d60037) SHA1(550aab8b0c8d85c9a69fe94ae042b6bb9f9d7db1))
ROM_END

/*
* MPCB828: EDGE-1 graphics, 1 megapixel, single screen, 60Hz refresh.
*
* Screen timings to match PCB963.
*
* "9 planes (1 for highlights) of double-buffered graphics, 256 colours from 16.7 million"
* 1024x1024x10x2 == 2621440 bytes == 2560KiB == 2.5MiB
* FIXME: diag reports 128KiB static ram, 1MiB video ram, 4 screens, 1 user, z-buffer absent
* 
*/
MACHINE_CONFIG_START(mpcb828_device::device_add_mconfig)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(83'020'800, 1504, 296 + 20, 1184 + 296 + 20, 920, 34, 884 + 34)
	//MCFG_SCREEN_RAW_PARAMS(83'020'800, 1184, 0, 1184, 884, 0, 884)
	MCFG_SCREEN_UPDATE_DEVICE(DEVICE_SELF, mpcb828_device, screen_update)
	MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(DEVICE_SELF, device_srx_card_interface, vblank))

	MCFG_DEVICE_ADD("sram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("128KiB")
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("vram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("2560KiB")
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("dsp", TMS32030, 1) // 30_MHz_XTAL
	MCFG_TMS3203X_HOLDA_CB(WRITELINE(DEVICE_SELF, mpcb828_device, holda))

	MCFG_DEVICE_ADD("ramdac", BT458, 83'020'800)

	MCFG_SCC8530_ADD("scc", 4.9152_MHz_XTAL, 0, 0, 0, 0)
	MCFG_Z80SCC_OUT_INT_CB(WRITELINE(DEVICE_SELF, mpcb828_device, scc_irq))
	MCFG_Z80SCC_OUT_TXDA_CB(WRITELINE("kbd", interpro_keyboard_port_device, write_txd))

	MCFG_INTERPRO_KEYBOARD_PORT_ADD("kbd", interpro_keyboard_devices, "hle_en_us")
	MCFG_INTERPRO_KEYBOARD_RXD_HANDLER(WRITELINE("scc", z80scc_device, rxa_w))
MACHINE_CONFIG_END

/*
 * MPCB849: EDGE-1 graphics, 2 megapixels, single screen, 60Hz refresh.
 *
 * System software gives visible pixels 1664x1248. Vertical refresh is assumed to
 * be 60Hz, horizontal sync ~55kHz.
 *
 * Inputs htotal=? (1664+?) and vtotal=? (1248+?) give hsync=?kHz.
 */
MACHINE_CONFIG_START(mpcb849_device::device_add_mconfig)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(164'609'300, 2112, 0, 1664, 1299, 0, 1248)
	MCFG_SCREEN_UPDATE_DEVICE(DEVICE_SELF, mpcb849_device, screen_update)
	MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(DEVICE_SELF, device_srx_card_interface, vblank))

	MCFG_DEVICE_ADD("sram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("128KiB")
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("vram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("5120KiB") // guess
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("dsp", TMS32030, 1) // 30_MHz_XTAL
	MCFG_TMS3203X_HOLDA_CB(WRITELINE(DEVICE_SELF, mpcb828_device, holda))

	MCFG_DEVICE_ADD("ramdac", BT458, 0) // unconfirmed clock

	MCFG_SCC8530_ADD("scc", 4.9152_MHz_XTAL, 0, 0, 0, 0)
	MCFG_Z80SCC_OUT_INT_CB(WRITELINE(DEVICE_SELF, mpcb849_device, scc_irq))
	MCFG_Z80SCC_OUT_TXDA_CB(WRITELINE("kbd", interpro_keyboard_port_device, write_txd))

	MCFG_INTERPRO_KEYBOARD_PORT_ADD("kbd", interpro_keyboard_devices, "hle_en_us")
	MCFG_INTERPRO_KEYBOARD_RXD_HANDLER(WRITELINE("scc", z80scc_device, rxa_w))
MACHINE_CONFIG_END

/*
 * MPCB030/MPCBA63: EDGE-2 graphics, 1 megapixel, single/dual screen, 60Hz refresh.
 *
 * System software gives visible pixels 1184x884. Vertical refresh is assumed to
 * be 60Hz, horizontal sync ~55kHz.
 *
 * Inputs htotal=? (1184+?) and vtotal=? (884+?) give hsync=?kHz.
 */
MACHINE_CONFIG_START(mpcb030_device::device_add_mconfig)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(mpcba63_device::device_add_mconfig)
	//MCFG_SCREEN_ADD("screen", RASTER)
	// screen params copied from GT
	//MCFG_SCREEN_RAW_PARAMS(83'020'800, 1504, 296 + 20, 1184 + 296 + 20, 920, 34, 884 + 34)
	//MCFG_SCREEN_UPDATE_DEVICE(DEVICE_SELF, mpcba63_device, screen_update)
	//MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(?, vblank))
	//MCFG_DEVICE_ADD("ramdac0", BT457, 0) // PS045701-165
	//MCFG_DEVICE_ADD("ramdac0", BT457, 0)
	//MCFG_DEVICE_ADD("ramdac0", BT457, 0)
MACHINE_CONFIG_END

/*
 * MSMT094/MPCB896: EDGE-2 Plus graphics, 2 megapixels, single screen, 60Hz refresh.
 *
 * System software gives visible pixels 1664x1248. Board documentation gives
 * pixel clock 164.6093MHz. Vertical refresh is assumed to be 60Hz.
 *
 * Inputs htotal=2112 (1664+448) and vtotal=1299 (1248+51) give hsync=77.940kHz.
 */
MACHINE_CONFIG_START(msmt094_device::device_add_mconfig)
	// FIXME: actually 33.333_MHz_XTAL
	MCFG_DEVICE_ADD("dsp1", TMS32030, 1)
	MCFG_TMS3203X_HOLDA_CB(WRITELINE(DEVICE_SELF, msmt094_device, holda))
	//MCFG_DEVICE_ADD("dsp2", TMS32030, 40_MHz_XTAL)
	//MCFG_DEVICE_ADD("dsp3", TMS32030, 40_MHz_XTAL)

	// FIXME: actually Z0853006VSC
	MCFG_SCC8530_ADD("scc", 4.9152_MHz_XTAL, 0, 0, 0, 0)
	MCFG_Z80SCC_OUT_INT_CB(WRITELINE(DEVICE_SELF, msmt094_device, scc_irq))
	MCFG_Z80SCC_OUT_TXDA_CB(WRITELINE("kbd", interpro_keyboard_port_device, write_txd))

	MCFG_INTERPRO_KEYBOARD_PORT_ADD("kbd", interpro_keyboard_devices, "hle_en_us")
	MCFG_INTERPRO_KEYBOARD_RXD_HANDLER(WRITELINE("scc", z80scc_device, rxa_w))
MACHINE_CONFIG_END

MACHINE_CONFIG_START(mpcb896_device::device_add_mconfig)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(164'609'300, 2112, 0, 1664, 1299, 0, 1248)
	MCFG_SCREEN_UPDATE_DEVICE(DEVICE_SELF, mpcb896_device, screen_update)
	MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(DEVICE_SELF, device_srx_card_interface, vblank))

	MCFG_DEVICE_ADD("sram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("256KiB")
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("vram", RAM, 0)
	MCFG_RAM_DEFAULT_SIZE("18MiB")
	MCFG_RAM_DEFAULT_VALUE(0)

	MCFG_DEVICE_ADD("ramdac0", BT457, 164'609'300)
	MCFG_DEVICE_ADD("ramdac1", BT457, 164'609'300)
	MCFG_DEVICE_ADD("ramdac2", BT457, 164'609'300)
MACHINE_CONFIG_END

edge1_device_base::edge1_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, srx_card_device_base(mconfig, *this)
	, m_screen(*this, "screen")
	, m_sram(*this, "sram")
	, m_vram(*this, "vram")
	, m_dsp(*this, "dsp")
	, m_ramdac(*this, "ramdac")
	, m_scc(*this, "scc")
{
}

edge2_processor_device_base::edge2_processor_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, srx_card_device_base(mconfig, *this)
{
}

edge2plus_processor_device_base::edge2plus_processor_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, srx_card_device_base(mconfig, *this)
	, m_dsp1(*this, "dsp1")
	, m_screen(nullptr)
{
}

edge2_framebuffer_device_base::edge2_framebuffer_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, srx_card_device_base(mconfig, *this)
{
}

edge2plus_framebuffer_device_base::edge2plus_framebuffer_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, srx_card_device_base(mconfig, *this)
	, m_screen(*this, "screen")
	, m_sram(*this, "sram")
	, m_vram(*this, "vram")
	, m_ramdac(*this, "ramdac%u", 0U)
{
}

mpcb828_device::mpcb828_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge1_device_base(mconfig, MPCB828, tag, owner, clock)
{
}

mpcb849_device::mpcb849_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge1_device_base(mconfig, MPCB849, tag, owner, clock)
{
}

mpcb030_device::mpcb030_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge2_processor_device_base(mconfig, MPCB030, tag, owner, clock)
{
}

mpcba63_device::mpcba63_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge2_framebuffer_device_base(mconfig, MPCBA63, tag, owner, clock)
{
}

msmt094_device::msmt094_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge2plus_processor_device_base(mconfig, MSMT094, tag, owner, clock)
{
}

mpcb896_device::mpcb896_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: edge2plus_framebuffer_device_base(mconfig, MPCB896, tag, owner, clock)
{
}

const tiny_rom_entry *mpcb828_device::device_rom_region() const
{
	return ROM_NAME(mpcb828);
}

const tiny_rom_entry *mpcb849_device::device_rom_region() const
{
	return ROM_NAME(mpcb849);
}

const tiny_rom_entry *mpcb030_device::device_rom_region() const
{
	return ROM_NAME(mpcb030);
}

const tiny_rom_entry *mpcba63_device::device_rom_region() const
{
	return ROM_NAME(mpcba63);
}

const tiny_rom_entry *msmt094_device::device_rom_region() const
{
	return ROM_NAME(msmt094);
}

const tiny_rom_entry *mpcb896_device::device_rom_region() const
{
	return ROM_NAME(mpcb896);
}

void edge1_device_base::device_start()
{
	set_bus_device();

	// FIXME: dynamically allocate SR region 4
	m_bus->install_map(*this, 0x90000000, 0x93ffffff, &edge1_device_base::map_dynamic);
}

void edge2_processor_device_base::device_start()
{
	set_bus_device();
}

void edge2plus_processor_device_base::device_start()
{
	set_bus_device();
}

void edge2_framebuffer_device_base::device_start()
{
	set_bus_device();
}

void edge2plus_framebuffer_device_base::device_start()
{
	set_bus_device();

	// find the processor board and notify it of our screen
	edge2plus_processor_device_base *processor = m_bus->get_card<edge2plus_processor_device_base>();
	if (processor != nullptr)
	{
		LOG("found processor device %s\n", processor->tag());

		processor->register_screen(m_screen);
	}

	// FIXME: dynamically allocate SR region 4
	m_bus->install_map(*this, 0x90000000, 0x93ffffff, &edge2plus_framebuffer_device_base::map_dynamic);
}

u32 mpcb849_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

u32 mpcba63_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

u32 edge1_device_base::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	u8 *pixel_data = m_vram->pointer();

	// 8bpp mode
	for (int y = screen.visible_area().min_y; y <= screen.visible_area().max_y; y++)
		for (int x = screen.visible_area().min_x; x <= screen.visible_area().max_x; x++)
			bitmap.pix(y, x) = m_ramdac->pen_color(*pixel_data++);

	return 0;
}

u32 edge2plus_framebuffer_device_base::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	u8 *pixel_data = m_vram->pointer();

	// 8bpp mode
	for (int y = screen.visible_area().min_y; y <= screen.visible_area().max_y; y++)
		for (int x = screen.visible_area().min_x; x <= screen.visible_area().max_x; x++)
		{
			const u8 index = *pixel_data++;

			bitmap.pix(y, x) = rgb_t(
				m_ramdac[0]->palette_lookup(index),
				m_ramdac[1]->palette_lookup(index),
				m_ramdac[2]->palette_lookup(index));
		}

	return 0;
}

WRITE32_MEMBER(edge1_device_base::control_w)
{
	COMBINE_DATA(&m_control);

	// release dsp 1 hold
	if (data & NO_HOLD)
		m_dsp->set_input_line(TMS3203X_HOLD, CLEAR_LINE);

	// hold dsp 1
	if (!(data & DSP_1_HOLD_L))
		m_dsp->set_input_line(TMS3203X_HOLD, ASSERT_LINE);
}

WRITE_LINE_MEMBER(edge1_device_base::holda)
{
	LOG("hold acknowledge %d\n", state);

	if (state)
	{
		m_status |= DSP_1_HOLDA_H;

		if (m_control & HOLD_ENB)
		{
			m_control |= HOLDA_INT_H;

			irq0(ASSERT_LINE);
		}
	}
	else
		m_status &= ~DSP_1_HOLDA_H;
}

WRITE32_MEMBER(edge2plus_processor_device_base::control_w)
{
	COMBINE_DATA(&m_control);

	// release dsp 1 hold
	if (data & NO_HOLD)
		m_dsp1->set_input_line(TMS3203X_HOLD, CLEAR_LINE);

	// hold dsp 1
	if (!(data & DSP_1_HOLD_L))
		m_dsp1->set_input_line(TMS3203X_HOLD, ASSERT_LINE);
}

WRITE_LINE_MEMBER(edge2plus_processor_device_base::holda)
{
	LOG("hold acknowledge %d\n", state);

	if (state)
	{
		m_status |= DSP_1_HOLDA_H;

		if (m_control & HOLD_ENB)
		{
			m_control |= HOLDA_INT_H;
			irq0(ASSERT_LINE);
		}
	}
	else
		m_status &= ~DSP_1_HOLDA_H;
}

WRITE32_MEMBER(edge2plus_framebuffer_device_base::lut_select_w)
{
	LOG("select ramdac %d\n", data);

	void (bt457_device::*map)(address_map &) = &bt457_device::map;

	// TODO: not sure what should happen for values > 2
	// FIXME: this should probably be some kind of bank device, including the idprom
	m_bus->main_space()->install_device(0x92028280U, 0x9202828fU, *m_ramdac[data % 3], map, 0xff, 8);
	m_bus->io_space()->install_device(0x92028280U, 0x9202828fU, *m_ramdac[data % 3], map, 0xff, 8);

	// lookup table 3 enables address range 92030000-92030fff, written with zeroes
}
