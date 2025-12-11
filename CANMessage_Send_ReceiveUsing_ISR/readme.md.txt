This project demonstrates CAN communication between an NXP MPC5748G microcontroller board and an external data logger. The application performs two key functions:
	Receives a CAN message from the data logger, increments its original CAN ID by +1, and re-transmits it back to the logger.
	Uses a PIT (Periodic Interrupt Timer) interrupt to send a fixed CAN message at a periodic interval (e.g., 50 ms) to the data logger.

#Project Overview
The firmware running on the MPC5748G includes:
1. CAN Receive → Modify → Transmit
	The board listens on a configured CAN channel.
	When a valid CAN frame is received:
		Extract the original CAN ID.
		Compute new_id = original_id + 1.
		Re-transmit the frame using the updated CAN ID.
		The payload/data bytes remain unchanged (unless otherwise configured).
2. Periodic CAN Message via PIT Interrupt
	A PIT timer (e.g. PIT0) is configured to generate interrupts at 50 ms.
	Inside the PIT ISR:
		A predefined CAN frame (fixed ID + fixed data) is transmitted periodically.
		The timer uses a system clock of 160 MHz, with LDVAL calculated to match the 50 ms period.


#Hardware & Software Requirements
	Hardware:
		NXP MPC5748G development board
		External CAN/CANFD data logger
		CAN transceivers (if required depending on board configuration)
		Proper CAN termination (120 Ω resistors)
	Software:
		S32 Design Studio / GCC-based toolchain
		NXP SDK or custom low-level drivers
		CAN peripheral driver (FlexCAN)
		PIT timer driver

#Communication Flow
	1. From Data Logger → MCU → Data Logger
		[Logger] --CAN--> [MPC5748G receives frame]
		[MPC5748G] increments ID → transmits back
		[Logger] receives updated frame (ID + 1)

	2. Periodic Transmission Using PIT
		[PIT Interrupt (50 ms)] → [MPC5748G sends fixed CAN message]

#Key Functional Blocks
	CAN Initialization
	Configure CAN controller (bit timing, mailbox setup)
	Enable CAN RX maibox for incoming frames
	Prepare TX mailbox for sending modified frames
	Triggered when new frame arrives
	Read CAN ID and data
	Modify CAN ID → ID = ID + 1
	Load transmit mailbox and send
	PIT Initialization
	PIT clock source: System clock (40 MHz)
	Load Value = (Period_in_seconds × Clock_frequency) - 1
	Enable PIT interrupt
	Register ISR
	PIT ISR  : Generate interrupt every 50 ms
	Transmit predefined fixed CAN frame

#Example CAN Behavior
	1) Incoming Message - ID: 0x100
	   Data: A0 12 23 45 56 76 87 94
	2) Outgoing Message After Modification - ID: 0x101
	   Data: unchanged
	3) Periodic Message from PIT - ID: 0x600
	   Data: 00 06 62 A5 01 FF FF FF
	   Sent every 50 ms

#Testing & Verification
	1) Connect the MPC5748G and logger on the same CAN network.
	2) Configure matching bitrates (500 kbps / 1 Mbps as required).
	3) Send a known CAN frame from the logger.
	4) Verify that:
		The board receives the frame.
        The returned frame has ID + 1.
	5) Monitor periodic fixed frames from the MCU every 50 ms.
	
#License
This project is using MIT License.