# Plan
- Update `tokens.ts` replacing `WARM_TOKENS` with `LAVENDER_TOKENS` and `DEV_TOKENS` with `STEEL_TOKENS`.
- Update `SettingsOverlay.tsx` theme selector to show Lavender and Steel.
- Adjust `BootAnimation.tsx` logic so Steel maps to hacker-style, and Lavender/Child to bouncy face.
- Adjust layout containers in `SettingsOverlay`, `ModelPicker`, `PersonaGrid`, and `Console` to remove the 30% transparency (`4D`) on light panels (Lavender/Child/Steel) and only apply it strictly to `Tech` to ensure deep contrast on white panels.
- Reply to user explaining the specific contrast upgrades and color decisions.