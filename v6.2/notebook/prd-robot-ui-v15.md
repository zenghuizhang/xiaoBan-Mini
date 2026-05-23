# M5Stack CoreS3 Companion Robot UI v15 (PRD v1.1)
- **Radial Menu**: Double-clicking the screen now opens a Radial Menu instead of the bottom bar. 6 sectors (Expressions, Dialogue, Settings, Theme, Extensions, Random) with Spring animations (damping:0.7 / ~17, stiffness:150).
- **Breathing Modes**: Implemented Modified Sine Curve logic using Framer Motion. Added `deep_sleep`, `light_rest`, `alert` to complement the existing `breath` state.
- **Somatosensory Matrix**: Mapped physical actions (Tilt Forward, Tilt Backward, Tilt Left, Tilt Right, Shake, Rotate) to expressions (`curious`, `yawn`, `thinking`, `surprised`, `dizzy`, `naughty`). Added these to the Scenario Simulator.
- **Text Display Zones**: Adjusted `DialogBubble` to appear at the top/bottom safe zones (X: 0-320, Y: 0-20 or 200-240) to strictly avoid blocking the eyes (Y: 60-180) and mouth (Y: 140-180).
- **RGB Light**: Refined to use Cyan (`#33C5FF` / `cyan-400`) for Tech theme and Coral (`#FF9E7D` / `orange-400`) for Child theme during breathing, with specific pulsing for alerts/rewards.
- Removed BottomBar in favor of pure HUD Radial Menu to align with the "no screen clutter" vision.