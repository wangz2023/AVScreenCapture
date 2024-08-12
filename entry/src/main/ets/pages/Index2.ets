import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';
  testTag: string = "wangz::";

  build() {
    Row() {
      Column() {
        Text(this.message)
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
          .onClick(() => {
            hilog.info(0x0000, this.testTag, 'Test NAPI 2 + 3 = %{public}d', testNapi.add(2, 3));
            hilog.info(0x0000, this.testTag, 'screenTest::' + testNapi.screenTest());
          })
      }
      .width('100%')
    }
    .height('100%')
  }
}


// import backgroundTaskManager from '@ohos.resourceschedule.backgroundTaskManager';
// import UIAbility from '@ohos.app.ability.UIAbility';
// import window from '@ohos.window';
// import AbilityConstant from '@ohos.app.ability.AbilityConstant';
// import Want from '@ohos.app.ability.Want';
// import rpc from '@ohos.rpc';
// import { BusinessError } from '@ohos.base';
// import wantAgent, { WantAgent } from '@ohos.app.ability.wantAgent';
//
// @Entry
// @Component
// struct Index {
//   @State message: string = 'ContinuousTask';
//   // 通过getContext方法，来获取page所在的UIAbility上下文。
//   private context: Context = getContext(this);
//
//   startContinuousTask() {
//     let wantAgentInfo: wantAgent.WantAgentInfo = {
//       // 点击通知后，将要执行的动作列表
//       // 添加需要被拉起应用的bundleName和abilityName
//       wants: [
//         {
//           bundleName: "com.example.myapplication",
//           abilityName: "com.example.myapplication.EntryAbility"
//         }
//       ],
//       // 指定点击通知栏消息后的动作是拉起ability
//       operationType: wantAgent.OperationType.START_ABILITY,
//       // 使用者自定义的一个私有值
//       requestCode: 0,
//       // 点击通知后，动作执行属性
//       wantAgentFlags: [wantAgent.WantAgentFlags.UPDATE_PRESENT_FLAG]
//     };
//
//     // 通过wantAgent模块下getWantAgent方法获取WantAgent对象
//     wantAgent.getWantAgent(wantAgentInfo).then((wantAgentObj: WantAgent) => {
//       backgroundTaskManager.startBackgroundRunning(this.context,
//         backgroundTaskManager.BackgroundMode.AUDIO_RECORDING, wantAgentObj).then(() => {
//         console.info(`wangz::Succeeded in operationing startBackgroundRunning.`);
//       }).catch((err: BusinessError) => {
//         console.error(`wangz::Failed to operation startBackgroundRunning. Code is ${err.code}, message is ${err.message}`);
//       });
//     });
//   }
//
//   stopContinuousTask() {
//     backgroundTaskManager.stopBackgroundRunning(this.context).then(() => {
//       console.info(`wangz::Succeeded in operationing stopBackgroundRunning.`);
//     }).catch((err: BusinessError) => {
//       console.error(`wangz::Failed to operation stopBackgroundRunning. Code is ${err.code}, message is ${err.message}`);
//     });
//   }
//
//   build() {
//     Row() {
//       Column() {
//         Text("Index")
//           .fontSize(50)
//           .fontWeight(FontWeight.Bold)
//
//         Button() {
//           Text('申请长时任务').fontSize(25).fontWeight(FontWeight.Bold)
//         }
//         .type(ButtonType.Capsule)
//         .margin({ top: 10 })
//         .backgroundColor('#0D9FFB')
//         .width(250)
//         .height(40)
//         .onClick(() => {
//           // 通过按钮申请长时任务
//           this.startContinuousTask();
//
//           // 此处执行具体的长时任务逻辑，如放音等。
//         })
//
//         Button() {
//           Text('取消长时任务').fontSize(25).fontWeight(FontWeight.Bold)
//         }
//         .type(ButtonType.Capsule)
//         .margin({ top: 10 })
//         .backgroundColor('#0D9FFB')
//         .width(250)
//         .height(40)
//         .onClick(() => {
//           // 此处结束具体的长时任务的执行
//
//           // 通过按钮取消长时任务
//           this.stopContinuousTask();
//         })
//       }
//       .width('100%')
//     }
//     .height('100%')
//   }
// }



