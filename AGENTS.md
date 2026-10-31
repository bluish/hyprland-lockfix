# AGENTS.md

`hyprland-lockfix` is one C++ file built as a Hyprland plugin. It hooks
`CSessionLockProtocol::onGetLockSurface` with `HyprlandAPI::createFunctionHook`
and creates an inert `ext_session_lock_surface_v1` when the output's monitor is
gone (hyprwm/Hyprland#16385).

## Rules

- Every `new_id` must get a server object. Never return early from the hook
  without creating one; that is the bug this plugin exists for.
- Check the monitor with `m_monitor.lock()`, like the original: a weak pointer's
  `operator bool` accepts a monitor that is being destroyed.
- Keep the hash check in `pluginInit`: a plugin built for another Hyprland can
  hook the wrong code.
- Build with `-fcf-protection=full`: Hyprland is built with IBT, and the hook
  jumps into the plugin indirectly.
- Before a new Hyprland version, check that `onGetLockSurface` is still
  exported and called through its entry (`objdump -d`), not inlined.

## Testing

- `make` must build without warnings.
- `hyprctl plugin load "$PWD/lockfix.so"` and `hyprctl plugin list`.
- The real case needs a monitor that drops hotplug while locked: lock, switch
  the monitor to another input, come back, and check the Hyprland log for
  `[lockfix]` and `journalctl --user` for the locker exiting.

## Commits

`type(scope): description` in English; body with why, then lowercase
bullets of what changed, then a `Validation:` line with what was actually run.
