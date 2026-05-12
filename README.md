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

- [ ] Implement a filestructure for the server files (HIGH)
- [ ] Polish the STA server (LOW)
- [ ] Expand Captive portal support (i.e. mac, android, windows, ~iOS~) (HIGH)
- [ ] Setup OTA firmware updates / patching
  - [ ] Over STA LAN (AP is more prone to external attackers) (HIGH)
  - [ ] Over Remote server (LOW)
    - This is pretty sick, streamlines setting up new boards and can do massive rollouts in parallel
    - Security actions: there should be a level of auth when connecting to the server and vice - versa
- [ ] Board communicating with a remote server (final goal) (LOW)
  - Sort of similar ideas -> [Building an AWS IoT Core device using AWS Serverless and an ESP32](https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/)
