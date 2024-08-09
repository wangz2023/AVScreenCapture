#include "screen_manager.h"
#include <multimedia/player_framework/native_avscreen_capture.h>

namespace OHOS_SCREEN_SAMPLE {
CapiScreen::CapiScreen(){};
CapiScreen::~CapiScreen() {}
void CapiScreen::Test() {
    if (!isRunning) {
        {
            CreateAndInit();
            Start();
            isRunning = true;
        }
    } else {
        {
            StopAndRelease();
            isRunning = false;
        }
    }
}

void CapiScreen::CreateAndInit(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::CreateAndInit");
    capture = OH_AVScreenCapture_Create();

    auto OnError = [](OH_AVScreenCapture *capture, int32_t errorCode, void *userData) {
        std::string errorCodeStr = std::to_string(errorCode);
        OH_LOG_DEBUG(LOG_APP, "wangz::OnError and errorCode::%s", errorCodeStr.c_str());
        (void)capture;
        (void)errorCode;
        (void)userData;
    };
    auto OnStateChange = [](struct OH_AVScreenCapture *capture, OH_AVScreenCaptureStateCode stateCode, void *userData) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnStateChange");
        (void)capture;

        if (stateCode == OH_SCREEN_CAPTURE_STATE_STARTED) {
            // 处理状态变更
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_STATE_STARTED");
        }
        if (stateCode == OH_SCREEN_CAPTURE_STATE_STOPPED_BY_CALL) {
            // 通话中断状态处理
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_STATE_STOPPED_BY_CALL");
        }
        if (stateCode == OH_SCREEN_CAPTURE_STATE_INTERRUPTED_BY_OTHER) {
            // 处理状态变更
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_STATE_INTERRUPTED_BY_OTHER");
        }
        (void)userData;
    };
    auto OnBufferAvailable = [](OH_AVScreenCapture *capture, OH_AVBuffer *buffer,
                                OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void *userData) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnBufferAvailable");
        // 获取解码后信息 可以参考编解码接口
        int bufferLen = OH_AVBuffer_GetCapacity(buffer);
        OH_NativeBuffer *nativeBuffer = OH_AVBuffer_GetNativeBuffer(buffer);
        OH_NativeBuffer_Config config;
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
        int32_t videoSize = config.height * config.width * 4;
        uint8_t *buf = OH_AVBuffer_GetAddr(buffer);
        if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO) {
            // 处理视频buffer
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
            // 处理内录buffer
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
            // 处理麦克风buffer
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC");
        }
    };

    // 设置回调
    OH_AVScreenCapture_SetErrorCallback(capture, OnError, nullptr);
    OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, nullptr);
    OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailable, nullptr);

    // 可选 配置录屏旋转，此接口在感知到手机屏幕旋转时调用，如果手机的屏幕实际上没有发生旋转，调用接口是无效的。
    OH_AVScreenCapture_SetCanvasRotation(capture, true);
    // 可选 [过滤音频]
    OH_AVScreenCapture_ContentFilter *contentFilter = OH_AVScreenCapture_CreateContentFilter();
    // 添加过滤通知音
    OH_AVScreenCapture_ContentFilter_AddAudioContent(contentFilter, OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO);
    // 排除过滤器
    OH_AVScreenCapture_ExcludeContent(capture, contentFilter);

    // 初始化录屏，传入配置信息OH_AVScreenRecorderConfig
    OH_AudioCaptureInfo miccapinfo = {.audioSampleRate = 16000, .audioChannels = 2, .audioSource = OH_MIC};
    OH_VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720, .videoFrameHeight = 1080, .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
    OH_AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
    };
    OH_VideoInfo videoinfo = {.videoCapInfo = videocapinfo};
    OH_AVScreenCaptureConfig config = {.captureMode = OH_CAPTURE_HOME_SCREEN,
                                       .dataType = OH_ORIGINAL_STREAM,
                                       .audioInfo = audioinfo,
                                       .videoInfo = videoinfo};
    OH_AVScreenCapture_Init(capture, config);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::CreateAndInit");
}

void CapiScreen::Start(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::Start");
    // 开始录屏
    OH_AVScreenCapture_StartScreenCapture(capture);
    // mic开关设置
    OH_AVScreenCapture_SetMicrophoneEnabled(capture, true);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::Start");
}

void CapiScreen::StopAndRelease(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StopAndRelease");
    // 结束录屏
    OH_AVScreenCapture_StopScreenCapture(capture);
    // 释放ScreenCapture
    OH_AVScreenCapture_Release(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::StopAndRelease");
}

} // namespace OHOS_SCREEN_SAMPLE