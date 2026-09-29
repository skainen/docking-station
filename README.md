# IR-Guided Docking Station

Autonomous docking for a mobile robot: the robot finds its station with an infrared beacon, aligns itself and makes verified electrical contact without human help.

IoT Project 2, Group 1

## Team

- Santeri Karjalainen
- Zohaib Salman

## Problem

The robot's batteries are removed and charged by hand. Availability depends on someone remembering to charge it, battery handling adds maintenance work, and the robot has no defined parking position.

## Goals

1. Build a 3D-printed docking station with an IR beacon the robot can home in on.
2. Implement robot-side IR sensing and a docking behaviour on STM32.
3. Dock in at least 8 of 10 trials from up to 2 m away and ±45° off-axis, under normal indoor lighting.
4. Detect and confirm electrical contact at the dock.

**Next version:** charging the batteries through the dock contacts. The contacts and wiring are sized for charging current, but the charging electronics are not part of this version.

## How it works

### IR guidance

- The dock's STM32 generates a 38 kHz carrier and a distinct code per zone: **Left**, **Right** and **Near**.
- Codes are time-multiplexed, so only one zone transmits at a time.
- A thin septum between the left and right LEDs gives a sharp centre line. The robot is on the centre line when it sees both L and R.
- The low-power **Near** code is only visible within about 30–40 cm and tells the robot to slow down and deploy its contacts.
- The robot decodes the codes with three integrated 38 kHz receivers (TSOP38238) and steers onto the centre line.

### Contacts

- **Dock:** a floor pad with two spring-loaded contact rails running along the approach direction. The rails give generous lateral and overrun tolerance.
- **Robot:** two contact rods on a servo-driven carrier, about 10 cm apart.
  - The rods are held horizontal while driving and rotated vertical on the final approach.
  - A hard stop at vertical takes the contact load, so the servo doesn't hold the dock spring force.
  - A microswitch confirms the rods are deployed.
  - Return springs lift the rods if the servo loses power.
- The robot detects a successful dock by sensing the contact voltage.

## Hardware

| Part | Role |
|---|---|
| NUCLEO-G031K8 | Dock MCU: IR carrier and zone codes |
| TSAL6200 IR LEDs + IRLZ44N MOSFETs | Beacon emitters, one switched channel per zone |
| NUCLEO-G071RB | Robot docking controller |
| TSOP38238 ×3 | Robot IR receivers |
| Metal-gear micro servo | Deploys the contact rods |
| D2F-01L microswitch | Confirms rods are deployed |
| Brass rails and rods, springs | Contacts |


## Repository layout

```
ir-docking/
├── README.md
├── docs/        # proposals, architecture, parts lists, test logs
├── dock-firmware/     # dock (beacon) firmware, STM32G031
├── robot-firmware/    # receiver + docking controller firmware, STM32G071
├── hardware/    # schematics, CAD for station, pad and rod mechanism
└── tools/       # IR logger, test scripts
```

## Milestones

| Milestone | Content |
|---|---|
| M1: IR link | Beacon sends L / R / Near codes; robot-side receivers decode them on the bench; ambient IR tested |
| M2: Steering | Docking state machine steers a test base onto the centre line from the defined area |
| M3: Contacts | Station, floor pad and rod mechanism printed and assembled; contact detection working |
| M4: Integration and demo | Full docking on the robot; 10-trial test from varied start positions |
