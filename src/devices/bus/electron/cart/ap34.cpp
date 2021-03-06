// license:BSD-3-Clause
// copyright-holders:Nigel Barnes
/**********************************************************************

    P.R.E.S. Advanced Plus3/4

    http://chrisacorns.computinghistory.org.uk/8bit_Upgrades/PRES_AP3A.html

    TODO:
    - AP4 (DFS) is unreliable, maybe WD1770 reset issue to be investigated
    - add spare ROM slot in AP3 and AP4, not AP3/4

**********************************************************************/


#include "emu.h"
#include "ap34.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(ELECTRON_AP34, electron_ap34_device, "electron_ap34", "P.R.E.S. Advanced Plus 3/4")


//-------------------------------------------------
//  MACHINE_DRIVER( ap34 )
//-------------------------------------------------

FLOPPY_FORMATS_MEMBER(electron_ap34_device::floppy_formats)
	FLOPPY_ACORN_SSD_FORMAT,
	FLOPPY_ACORN_DSD_FORMAT,
	FLOPPY_ACORN_ADFS_OLD_FORMAT
FLOPPY_FORMATS_END0

void ap34_floppies(device_slot_interface &device)
{
	device.option_add("35dd",  FLOPPY_35_DD);
	device.option_add("525qd", FLOPPY_525_QD);
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

MACHINE_CONFIG_START(electron_ap34_device::device_add_mconfig)
	/* fdc */
	MCFG_WD1770_ADD("fdc", 16_MHz_XTAL / 2)
	MCFG_FLOPPY_DRIVE_ADD("fdc:0", ap34_floppies, "525qd", electron_ap34_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)
	MCFG_FLOPPY_DRIVE_ADD("fdc:1", ap34_floppies, nullptr, electron_ap34_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)
MACHINE_CONFIG_END

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  electron_ap34_device - constructor
//-------------------------------------------------

electron_ap34_device::electron_ap34_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, ELECTRON_AP34, tag, owner, clock)
	, device_electron_cart_interface(mconfig, *this)
	, m_fdc(*this, "fdc")
	, m_floppy0(*this, "fdc:0")
	, m_floppy1(*this, "fdc:1")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void electron_ap34_device::device_start()
{
}

//-------------------------------------------------
//  read - cartridge data read
//-------------------------------------------------

uint8_t electron_ap34_device::read(address_space &space, offs_t offset, int infc, int infd, int romqa)
{
	uint8_t data = 0xfe;

	if (infc)
	{
		switch (offset & 0xff)
		{
		case 0xc4:
		case 0xc5:
		case 0xc6:
		case 0xc7:
			data = m_fdc->read(space, offset & 0x03);
			break;
		}
	}

	if (!infc && !infd)
	{
		if (offset >= 0x0000 && offset < 0x4000)
		{
			data = m_rom[(offset & 0x3fff) + (romqa * 0x4000)];
		}

		if (m_ram.size() != 0 && romqa == 0 && offset >= 0x3000)
		{
			data = m_ram[offset & 0x0fff];
		}
	}

	return data;
}

//-------------------------------------------------
//  write - cartridge data write
//-------------------------------------------------

void electron_ap34_device::write(address_space &space, offs_t offset, uint8_t data, int infc, int infd, int romqa)
{
	if (infc)
	{
		switch (offset & 0xff)
		{
		case 0xc0:
			wd1770_control_w(space, 0, data);
			break;
		case 0xc4:
		case 0xc5:
		case 0xc6:
		case 0xc7:
			m_fdc->write(space, offset & 0x03, data);
			break;
		}
	}

	if (!infc && !infd)
	{
		if (m_ram.size() != 0 && romqa == 0 && offset >= 0x3000)
		{
			m_ram[offset & 0x0fff] = data;
		}
	}
}


//**************************************************************************
//  IMPLEMENTATION
//**************************************************************************

WRITE8_MEMBER(electron_ap34_device::wd1770_control_w)
{
	floppy_image_device *floppy = nullptr;

	// bit 0, 1: drive select
	if (BIT(data, 0)) floppy = m_floppy0->get_device();
	if (BIT(data, 1)) floppy = m_floppy1->get_device();
	m_fdc->set_floppy(floppy);

	// bit 2: side select
	if (floppy)
		floppy->ss_w(BIT(data, 2));

	// bit 3: density
	m_fdc->dden_w(BIT(data, 3));

	// bit 4: NMI - not connected

	// bit 5: reset
	if (!BIT(data, 5)) m_fdc->soft_reset();
}
