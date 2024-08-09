#include "napi/native_api.h"
#include "screen_manager.h"
static bool isRunning = false;
std::shared_ptr<OHOS_SCREEN_SAMPLE::CapiScreen> screen = std::make_shared<OHOS_SCREEN_SAMPLE::CapiScreen>();
static napi_value ScreenTest(napi_env env, napi_callback_info info) {
    screen->Test();


//     // 实例化ScreenCapture
//     capture = OH_AVScreenCapture_Create();
//
//     // 设置回调
//     OH_AVScreenCapture_SetErrorCallback(capture, OnError, nullptr);
//     OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, nullptr);
//     OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailable, nullptr);
//
//     // 可选 配置录屏旋转，此接口在感知到手机屏幕旋转时调用，如果手机的屏幕实际上没有发生旋转，调用接口是无效的。
//     OH_AVScreenCapture_SetCanvasRotation(capture, true);
//     // 可选 [过滤音频]
//     OH_AVScreenCapture_ContentFilter *contentFilter = OH_AVScreenCapture_CreateContentFilter();
//     // 添加过滤通知音
//     OH_AVScreenCapture_ContentFilter_AddAudioContent(contentFilter, OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO);
//     // 排除过滤器
//     OH_AVScreenCapture_ExcludeContent(capture, contentFilter);
//
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
//     OH_AVScreenCapture_Init(capture, config);
//
//     // 可选 [Surface模式]
//     // 通过 MIME TYPE 创建编码器，系统会根据MIME创建最合适的编码器。
//     OH_AVCodec *codec = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
//     // 从视频编码器获取输入Surface
// //     OH_AVErrCode OH_VideoEncoder_GetSurface(codec, window);
//     // 启动编码器
//     int32_t retEnc = OH_VideoEncoder_Start(codec);
//     // 指定surface开始录屏
// //     int32_t retStart = OH_AVScreenCapture_StartScreenCaptureWithSurface(capture, window);
//
//     // 开始录屏
//     OH_AVScreenCapture_StartScreenCapture(capture);
//     // mic开关设置
//     OH_AVScreenCapture_SetMicrophoneEnabled(capture, true);
//
//     sleep(100); // 录制10s
//     // 结束录屏
//     OH_AVScreenCapture_StopScreenCapture(capture);
//     // 释放ScreenCapture
//     OH_AVScreenCapture_Release(capture);
//     // 返回调用结果，示例仅返回随意值
    napi_value result;
    napi_create_int64(env, 111, &result);
    return result;
}


static napi_value Add(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"screenTest", nullptr, ScreenTest, nullptr, nullptr, nullptr, napi_default, nullptr}};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&demoModule); }
