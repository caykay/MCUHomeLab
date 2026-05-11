# HomeLab

## How it works?

- MCU configured to connect to wifi via a captive portal running on the esp32 board Access Point server
- Once connected to wifi there's two modes:
  - Can communicate with the controlled devices over the local network
  - Can communicate with the controlled devices over the internet by having the MCU send requests to a remote server

I switched to the two modes after realizing how complicated it is to host a secure http server on an esp32 that is exposed to the internet.
It felt easier to just have a secure server storing the esp32's device states.

Both approaches will need authentication

Basically, user can go locally which then the MCU will not send data to the remote server until after some inactivity from the local network.
MCU will periodically query the remote server on any state changes i.e. user requested a shutdown remotely, then updates its local state.

NOTE: this is a WIP documentation

## TODOs

- [ ] Implement a filestructure for the server files
- [ ] Polish the STA server
- [ ] Expand Captive portal support (i.e. mac, android, windows, ~iOS~)
- [ ] Setup OTA firmware updates (LOW)
