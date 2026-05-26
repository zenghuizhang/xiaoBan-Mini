## Iteration - Restore Somatosensory and Scenarios, Modify Boot Animation
- UI Changes: Restored "体感测试" (Somatosensory Test) with 6 actions in the Console's "System" tab.
- UI Changes: Restored "场景模拟" (Scenario Simulator) with 6 preset scenarios in the Console's "Service" tab.
- UI Changes: Modified the boot animation to remove the circular glasses shape, aligning the eye shape with the default expression (16px border-radius), and removed the blinking star (sparkles) effect for a cleaner look.
- Routing Changes: Updated `prototype-route.json` to match the new Console tab structure for system and service triggers.

## Iteration - Developer unlock and Chinese translation
- UI Changes: Developer options in Settings are now visible by default (removed the 5-tap to unlock logic).
- UI Changes: Completely translated all remaining English texts in Settings, Console, Model Picker, Theme Picker, and Memory Browser to professional Chinese terms.
- Logic Changes: Removed the `dev_mode` locking state from SettingsOverlay.