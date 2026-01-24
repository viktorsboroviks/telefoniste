- simple ipc handler
- fast, multi-client
- as simple as possible, but not simpler
  - very stable to work as intended from first try,
    but minimal on supported functionality and checks
  - extra features to be added as needed
- platforms: mac, linux
- single object
  - registers ipc interface
  - constantly listens on it
  - if new message
    - reads it
    - spawns a new thread
    - triggers registered callback
    - passes whatever was read to it
    - writes back response from the callback
- uses unix sockets
- in the future can be extended to use other ipc mechanism if needed
  (e.g. win support, net sockets)
