#include "screen_manager.h"
#include <hilog/log.h>


namespace OHOS_SCREEN_SAMPLE {
CapiScreen::CapiScreen(){};
CapiScreen::~CapiScreen() {}
void CapiScreen::Test() {
    if (!isRunning) {
        {
            CreateAndInit();
//             Start();
            StartWithSurfaceMode();
            isRunning = true;
        }
    } else {
        {
//             StopAndRelease();
            StopAndReleaseWithSurfaceMode();
            isRunning = false;
        }
    }
}

void CapiScreen::CreateAndInit(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::CreateAndInit");
    capture = OH_AVScreenCapture_Create();

    auto OnError = [](OH_AVScreenCapture *capture, int32_t errorCode, void *userData) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnError and errorCode::%{public}d", errorCode);
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
            //             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
            // 处理内录buffer
            //             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
            // 处理麦克风buffer
            //             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC");
        }
    };
    auto OnBufferAvailableWithSurfaceMode = [](OH_AVScreenCapture *capture, OH_AVBuffer *buffer,
                                OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void *userData) {
        
        OH_LOG_DEBUG(LOG_APP, "wangz::OnBufferAvailableWithSurfaceMode");
        // 获取解码后信息 可以参考编解码接口
        int bufferLen = OH_AVBuffer_GetCapacity(buffer);
        OH_NativeBuffer *nativeBuffer = OH_AVBuffer_GetNativeBuffer(buffer);
        OH_NativeBuffer_Config config;
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
        int32_t videoSize = config.height * config.width * 4;
        uint8_t *buf = OH_AVBuffer_GetAddr(buffer);
        if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO) {
            // 处理视频buffer
//             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
            // 处理内录buffer
//             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER");
        } else if (bufferType == OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
            // 处理麦克风buffer
//             OH_LOG_DEBUG(LOG_APP, "wangz::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC");
        }
        // 获取编码后信息
        auto *date = reinterpret_cast<OnBufferAvailableData*>(userData);
        OH_AVCodecBufferAttr info;
        int32_t ret = OH_AVBuffer_GetBufferAttr(buffer, &info);
        if (ret != AV_ERR_OK) {
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVBuffer_GetBufferAttr::failed");
        }
        // 将编码完成帧数据buffer写入到对应输出文件中
        date->outputFileRef->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), info.size);
        // 释放已完成写入的数据，index为对应输出队列下标
        int32_t retFreeOutputBuffer = OH_VideoEncoder_FreeOutputBuffer(date->codec, info.offset);
        if (retFreeOutputBuffer != AV_ERR_OK) {
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_FreeOutputBuffer::failed");
        }
    };

    // 设置回调
    //     OH_AVScreenCapture_SetErrorCallback(capture, OnError, nullptr);
    //     OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, nullptr);
    //     OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailable, nullptr);

    // 可选 配置录屏旋转，此接口在感知到手机屏幕旋转时调用，如果手机的屏幕实际上没有发生旋转，调用接口是无效的。
    OH_AVScreenCapture_SetCanvasRotation(capture, true);
    // 可选 [过滤音频]
    OH_AVScreenCapture_ContentFilter *contentFilter = OH_AVScreenCapture_CreateContentFilter();
    // 添加过滤通知音
    OH_AVScreenCapture_ContentFilter_AddAudioContent(contentFilter, OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO);
    // 排除过滤器
    OH_AVScreenCapture_ExcludeContent(capture, contentFilter);

//     // 初始化录屏，传入配置信息OH_AVScreenRecorderConfig
//     OH_AudioCaptureInfo miccapinfo = {.audioSampleRate = 16000, .audioChannels = 2, .audioSource = OH_MIC};
//     OH_VideoCaptureInfo videocapinfo = {
//         .videoFrameWidth = 720, .videoFrameHeight = 1080, .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
//     OH_AudioInfo audioinfo = {
//         .micCapInfo = miccapinfo,
//     };
//     OH_VideoInfo videoinfo = {.videoCapInfo = videocapinfo};
//     OH_AVScreenCaptureConfig config = {.captureMode = OH_CAPTURE_HOME_SCREEN,
//                                        .dataType = OH_ORIGINAL_STREAM,
//                                        .audioInfo = audioinfo,
//                                        .videoInfo = videoinfo};


    OH_AudioCaptureInfo miccapinfo = {.audioSampleRate = 16000, .audioChannels = 2, .audioSource = OH_MIC};
    OH_AudioCaptureInfo innerCapInfo = {.audioSampleRate = 16000, .audioChannels = 2, .audioSource = OH_ALL_PLAYBACK};
    OH_AudioEncInfo audioEncInfo = {.audioBitrate = 16000, .audioCodecformat = OH_AudioCodecFormat::OH_AAC_LC};

    OH_VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720, .videoFrameHeight = 1080, .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
    OH_VideoEncInfo videoEncInfo = {
        .videoCodec = OH_VideoCodecFormat::OH_H264, .videoBitrate = 2000000, .videoFrameRate = 30};

    OH_AudioInfo audioinfo = {.micCapInfo = miccapinfo, .innerCapInfo = innerCapInfo, .audioEncInfo = audioEncInfo};
    OH_VideoInfo videoinfo = {.videoCapInfo = videocapinfo, .videoEncInfo = videoEncInfo};
    OH_AVScreenCaptureConfig config = {.captureMode = OH_CAPTURE_HOME_SCREEN,
                                       .dataType = OH_ORIGINAL_STREAM,
                                       .audioInfo = audioinfo,
                                       .videoInfo = videoinfo};


    //         OH_AVScreenCaptureConfig config;
    //         OH_AudioCaptureInfo micCapInfo = {.audioSampleRate = 48000, .audioChannels = 2, .audioSource = OH_MIC};
    //
    //         OH_AudioCaptureInfo innerCapInfo = {.audioSampleRate = 48000, .audioChannels = 2, .audioSource =
    //         OH_ALL_PLAYBACK};
    //
    //         OH_AudioEncInfo audioEncInfo = {.audioBitrate = 48000, .audioCodecformat =
    //         OH_AudioCodecFormat::OH_AAC_LC};
    //
    //         OH_VideoCaptureInfo videoCapInfo = {
    //             .videoFrameWidth = 720, .videoFrameHeight = 1080, .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
    //
    //         OH_VideoEncInfo videoEncInfo = {
    //             .videoCodec = OH_VideoCodecFormat::OH_H264, .videoBitrate = 2000000, .videoFrameRate = 30};
    //
    //         OH_AudioInfo audioInfo = {.micCapInfo = micCapInfo, .innerCapInfo = innerCapInfo, .audioEncInfo =
    //         audioEncInfo};
    //
    //         OH_VideoInfo videoInfo = {.videoCapInfo = videoCapInfo, .videoEncInfo = videoEncInfo};
    //
    //         config = {
    //             .captureMode = OH_CAPTURE_HOME_SCREEN,
    //             .dataType = OH_CAPTURE_FILE,
    //             .audioInfo = audioInfo,
    //             .videoInfo = videoInfo,
    //         };
    //
    //         // 初始化录屏参数，传入配置信息OH_AVScreenRecorderConfig
    //         OH_RecorderInfo recorderInfo;
    //         const std::string SCREEN_CAPTURE_ROOT = "/data/storage/el2/base/files/";
    //         int32_t outputFd = open((SCREEN_CAPTURE_ROOT + "screen01.mp4").c_str(), O_RDWR | O_CREAT, 0777);
    //         std::string fileUrl = "fd://" + std::to_string(outputFd);
    //         recorderInfo.url = const_cast<char *>(fileUrl.c_str());
    //         recorderInfo.fileFormat = OH_ContainerFormatType::CFT_MPEG_4;
    //         config.recorderInfo = recorderInfo;


    OH_AVScreenCapture_SetErrorCallback(capture, OnError, nullptr);
    OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, nullptr);
    OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailable, nullptr);
    //     OnBufferAvailableData *date = new OnBufferAvailableData{this->outputFile, this->codec};
    //     OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailableWithSurfaceMode, date);

    //     OH_NativeBuffer* buffer = OH_AVScreenCapture_AcquireVideoBuffer(capture, &fence, &timestamp, &damage);
      

    int32_t retInit = OH_AVScreenCapture_Init(capture, config);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::OH_AVScreenCapture_Init::%{public}d", retInit);
}

void CapiScreen::StartWithSurfaceMode(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode");
    // 通过 MIME TYPE 创建编码器，系统会根据MIME创建最合适的编码器。
    codec = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);

    OHNativeWindow *nativeWindow;
    // 从视频编码器获取输入Surface
    OH_AVErrCode ret = OH_VideoEncoder_GetSurface(codec, &nativeWindow);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_GetSurface::%{public}d", ret);
    int32_t retPrepare = OH_VideoEncoder_Prepare(codec);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Prepare::%{public}d", retPrepare);

//     // 配置待编码文件路径
//     std::string_view outputFilePath = "/data/storage/el2/base/files/screen01.h264";
//     outputFile->open(outputFilePath.data(), std::ios::out | std::ios::binary | std::ios::ate);

    // 启动编码器
    int32_t retEnc = OH_VideoEncoder_Start(codec);
    if (retEnc != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Start::failed");
    }
    // 指定surface开始录屏
    int32_t retStart = OH_AVScreenCapture_StartScreenCaptureWithSurface(capture, nativeWindow);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVScreenCapture_StartScreenCaptureWithSurface::%{public}d", retStart);
    // mic开关设置
    OH_AVScreenCapture_SetMicrophoneEnabled(capture, true);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::StartWithSurfaceMode");
}

void CapiScreen::Start(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::Start");
    // 开始录屏
    int32_t retStart = OH_AVScreenCapture_StartScreenCapture(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVScreenCapture_StartScreenCapture::%{public}d", retStart);
    // mic开关设置
    OH_AVScreenCapture_SetMicrophoneEnabled(capture, true);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::Start");
}

void CapiScreen::StopAndReleaseWithSurfaceMode(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StopAndReleaseWithSurfaceMode");
    
    // 刷新编码器videoEnc
    int32_t ret = OH_VideoEncoder_Flush(codec);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Flush::failed");
    }
    // 重新开始编码
    ret = OH_VideoEncoder_Start(codec);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Start::failed");
    }

    // 终止编码器videoEnc
    int32_t retStop = OH_VideoEncoder_Stop(codec);
    if (retStop != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Stop::failed");
    }

    // 调用OH_VideoEncoder_Destroy，注销编码器
    int32_t retDestroy = OH_VideoEncoder_Destroy(codec);
    codec = nullptr;
    if (retDestroy != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Stop::failed");
    }

    // 结束录屏
    int32_t retStopScreenCapture = OH_AVScreenCapture_StopScreenCapture(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVScreenCapture_StopScreenCapture::%{public}d", retStopScreenCapture);
    // 释放ScreenCapture
    OH_AVScreenCapture_Release(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::StopAndReleaseWithSurfaceMode");
}

void CapiScreen::StopAndRelease(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StopAndRelease");
    // 结束录屏
    int32_t retStop = OH_AVScreenCapture_StopScreenCapture(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVScreenCapture_StopScreenCapture::%{public}d", retStop);
    // 释放ScreenCapture
    OH_AVScreenCapture_Release(capture);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::StopAndRelease");
}

} // namespace OHOS_SCREEN_SAMPLE