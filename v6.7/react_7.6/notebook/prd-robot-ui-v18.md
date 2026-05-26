# M5Stack CoreS3 Companion Robot UI v18
- Fully integrated the Developer Tools (IMU & Scenario simulation) into the `Console` under the `Service` tab.
- Added 3 new preset scenarios:
  1. `voice_wake` (语音唤醒): Excited expression, "我在听..." dialog, cyan strip.
  2. `ota` (OTA模拟): Deep sleep expression, "OTA 系统更新中..." dialog, purple pulsing strip.
  3. `error` (报错状态): Dizzy expression, "系统发生异常错误！" dialog, red fast pulsing strip.
- Discarded standalone `ScenarioOverlay`.
- Maintained 320x240 size, dark/cyan tech theme with stroke 2.4 icons. UI Language: Chinese.