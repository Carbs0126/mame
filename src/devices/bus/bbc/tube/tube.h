// license:BSD-3-Clause
// copyright-holders:Nigel Barnes
/**********************************************************************

    BBC Tube emulation

**********************************************************************

  Pinout:

             0V   1   2  R/NW (read/not-write)
             0V   3   4  2MHzE
             0V   5   6  NIRQ (not-interrupt request)
             0V   7   8  NTUBE
             0V   9  10  NRST (not-reset)
             0V  11  12  D0
             0V  13  14  D1
             0V  15  16  D2
             0V  17  18  D3
             0V  19  20  D4
             0V  21  22  D5
             0V  23  24  D6
             0V  25  26  D7
             0V  27  28  A0
             0V  29  30  A1
            +5V  31  32  A2
            +5V  33  34  A3
            +5V  35  36  A4
            +5V  37  38  A5
            +5V  39  40  A6

**********************************************************************/

#ifndef MAME_BUS_BBC_TUBE_TUBE_H
#define MAME_BUS_BBC_TUBE_TUBE_H

#pragma once



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define BBC_TUBE_SLOT_TAG      "tube"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_BBC_TUBE_SLOT_ADD(_tag, _slot_intf, _def_slot) \
	MCFG_DEVICE_ADD(_tag, BBC_TUBE_SLOT, 0) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, false)

#define MCFG_BBC_TUBE_SLOT_IRQ_HANDLER(_devcb) \
	devcb = &downcast<bbc_tube_slot_device &>(*device).set_irq_handler(DEVCB_##_devcb);


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> bbc_tube_slot_device

class device_bbc_tube_interface;

class bbc_tube_slot_device : public device_t, public device_slot_interface
{
public:
	// construction/destruction
	bbc_tube_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	virtual ~bbc_tube_slot_device();

	// callbacks
	template <class Object> devcb_base &set_irq_handler(Object &&cb) { return m_irq_handler.set_callback(std::forward<Object>(cb)); }

	DECLARE_READ8_MEMBER( host_r );
	DECLARE_WRITE8_MEMBER( host_w );

	DECLARE_WRITE_LINE_MEMBER( irq_w ) { m_irq_handler(state); }

protected:
	// device-level overrides
	virtual void device_validity_check(validity_checker &valid) const override;
	virtual void device_start() override;
	virtual void device_reset() override;

	device_bbc_tube_interface *m_card;

private:
	devcb_write_line m_irq_handler;
};


// ======================> device_bbc_tube_interface

class device_bbc_tube_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	virtual ~device_bbc_tube_interface();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(host_r) { return 0xfe; }
	virtual DECLARE_WRITE8_MEMBER(host_w) { }

protected:
	device_bbc_tube_interface(const machine_config &mconfig, device_t &device);

	bbc_tube_slot_device *m_slot;
};


// device type definition
DECLARE_DEVICE_TYPE(BBC_TUBE_SLOT, bbc_tube_slot_device)

void bbc_extube_devices(device_slot_interface &device);
void bbc_intube_devices(device_slot_interface &device);
//void bbc_x25tube_devices(device_slot_interface &device);


#endif // MAME_BUS_BBC_TUBE_TUBE_H
