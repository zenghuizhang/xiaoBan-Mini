## Iteration - Merge Setting Panel
- Logic Changes: Added a complete Settings Overlay with Wi-Fi, Voice, Brightness, Volume, and Sleep Timer controls.
- Routing Changes: Menu 'Settings' sector now points to `settings` instead of `wifi_ap`. `SettingsOverlay` points to `wifi_ap` for network configuration.
- UI Changes: Merged the two v6.1 branches. Kept the fixed right-tilt logic (`look_left` & `look_right`) from the main codebase while adopting the `SettingsOverlay` from the reference prototype.