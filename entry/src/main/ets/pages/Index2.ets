import backgroundTaskManager from '@ohos.resourceschedule.backgroundTaskManager';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import rpc from '@ohos.rpc';
import { BusinessError } from '@ohos.base';
import wantAgent, { WantAgent } from '@ohos.app.ability.wantAgent';
import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';

@Entry
@Component
struct Index {
  private context: Context = getContext(this);

  onPageShow(): void {
    hilog.info(0x0000, "wangz::", 'screenTest1111111::' + testNapi.screenTest());
  }

  build() {
    Row() {
      Column() {
        Text("11111111111111111111111111")
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
      }
      .width('100%')
    }
    .height('100%')
  }
}



