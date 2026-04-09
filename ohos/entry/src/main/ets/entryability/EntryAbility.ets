import UIAbility from '@ohos.app.ability.UIAbility';
import hilog from '@ohos.hilog';
import window from '@ohos.window';

export default class EntryAbility extends UIAbility {
  onCreate(want: object, launchParam: object) {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onCreate');
  }

  onDestroy() {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onWindowStageCreate');
    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        hilog.error(0x0000, 'MDKPlayer', 'Failed to load content: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Content loaded successfully');
    });
  }

  onWindowStageDestroy() {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    hilog.info(0x0000, 'MDKPlayer', '%{public}s', 'Ability onBackground');
  }
}
