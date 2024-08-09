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
