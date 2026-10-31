# hyprland-lockfix

Keep the screen locker alive when a monitor disappears while the session is
locked, instead of falling back to Hyprland's "lockscreen app died" screen.

A Hyprland plugin for
[hyprwm/Hyprland#16385](https://github.com/hyprwm/Hyprland/issues/16385). It was
written for Omarchy, where the locker is the omarchy-shell (Quickshell), but the
bug is in Hyprland and hits any ext-session-lock client.

## The problem

Lock the screen, then switch the monitor to another input (a KVM setup) or let it
drop hotplug in standby. Sometimes the locker dies, Hyprland shows its failsafe
screen, and the shell comes back a few seconds later.

What happens:

1. The monitor drops hotplug. Hyprland has no outputs left and creates its
   `FALLBACK` output.
2. The locker sees the new output and asks for a lock surface on it
   (`ext_session_lock_v1.get_lock_surface`).
3. The monitor comes back before Hyprland handles that request, and `FALLBACK`
   is removed.
4. Hyprland finds no monitor behind the output and returns without creating the
   object for the request's `new_id`.
5. The locker destroys the surface it thinks it has. Hyprland answers
   `invalid_object` and kills the client.

Hyprland 0.56.2 and 0.57.0 both have the early return in
`CSessionLockProtocol::onGetLockSurface`.

## The fix

The plugin hooks `CSessionLockProtocol::onGetLockSurface`. When the output is
invalid or its monitor is gone, it creates an inert lock surface for the
`new_id` that only accepts `destroy()`; otherwise it calls the original. The
locker's `destroy()` then succeeds and it keeps running.

It does not cover a lock surface requested after the lock itself finished: that
check happens before the hooked function.

## Install

```sh
sh install.sh
```

Then load it from `~/.config/hypr/hyprland.lua`
([`examples/hyprland.lua`](examples/hyprland.lua)). Needs a C++ compiler and the
Hyprland headers (shipped with the `hyprland` package on Arch).

Rebuild after every Hyprland update: the plugin refuses to load on another
version. Keep the `hl.plugin.load` line while a lock is up; unloading the plugin
destroys the inert surfaces the locker may still be holding.

When it acts, the Hyprland log shows
`[lockfix] get_lock_surface for a gone output: created inert lock surface <id>`.

## Tested on

- Hyprland 0.56.2 (aquamarine 0.15), Omarchy 4.5
- an HDMI 2.1 monitor shared with two Macs through DDC/CI input switching

## License

MIT
