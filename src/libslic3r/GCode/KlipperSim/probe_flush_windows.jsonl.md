Temporary notes for the current 1:1 port audit.

- `failed.gcode`
  - no flush slicing:
    - `offline_steps=649786`
    - `offline_cmds=19998`
    - `online_steps=649786`
    - `online_cmds=182085`
  - with runtime-style `flush_to(mcu_flush_time)` after every window:
    - `window_cmds=182085`
    - `window_b_cmds=166555`
- Key conclusion:
  - command explosion is coupled to the current streaming/flush semantics
  - next required check is against Klipper original chelper, not more guessing
