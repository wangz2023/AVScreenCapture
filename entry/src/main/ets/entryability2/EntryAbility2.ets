import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';

export default class EntryAbility2 extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onCreate');
  }

  onDestroy(): void {
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // Main window is created, set main page for this ability
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onWindowStageCreate');

    windowStage.loadContent('pages/Index2', (err) => {
      if (err.code) {
        hilog.error(0x0000, 'wangz::', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
      hilog.info(0x0000, 'wangz::', 'Succeeded in loading the content.');
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onWindowStageDestroy');
  }

  onForeground(): void {
    // Ability has brought to foreground
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    hilog.info(0x0000, 'wangz::', '%{public}s', 'Ability2 onBackground');
  }
}
