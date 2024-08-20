#include "napi/native_api.h"
#include "screen_manager.h"
#include <mutex>
#include <queue>
#include <hilog/log.h>
#include <fcntl.h>
#include "string"
#include "unistd.h"
#include <fstream>

namespace {
using namespace std::chrono_literals;
}

bool isRunning = false;
OH_AVScreenCapture *capture;
OH_AVCodec *codec;
OH_AVCodec *micCodec;

std::string_view outputFilePath = "/data/storage/el2/base/haps/entry/files/screen01.mp4";
std::unique_ptr<std::ofstream> outputFile = std::make_unique<std::ofstream>();

OH_AVFormat *format;
napi_threadsafe_function tsFn;
bool tsFnReadyLock = false;
std::mutex tsFnReadyMtx;
std::condition_variable tsFnReadyCondition;

class CodecUserData {
public:
    uint32_t inputFrameCount_ = 0;
    std::mutex inputMutex_;
    std::condition_variable inputCond_;
    std::queue<OH_AVBuffer *> inputBufferInfoQueue_;
    std::queue<uint32_t> inQueue_;

    uint32_t outputFrameCount_ = 0;
    std::mutex outputMutex_;
    std::condition_variable outputCond_;
    std::queue<OH_AVBuffer *> outputBufferInfoQueue_;
    std::queue<uint32_t> outQueue_;

    void ClearQueue() {
        {
            std::unique_lock<std::mutex> lock(inputMutex_);
            auto emptyQueue = std::queue<OH_AVBuffer *>();
            inputBufferInfoQueue_.swap(emptyQueue);
        }
        {
            std::unique_lock<std::mutex> lock(outputMutex_);
            auto emptyQueue = std::queue<OH_AVBuffer *>();
            outputBufferInfoQueue_.swap(emptyQueue);
        }
    }
};

struct OnBufferAvailableData {
    std::unique_ptr<std::ofstream> &outputFileRef;
    OH_AVCodec *codec;
};

// 初始化队列
class AEncBufferSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::mutex startMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::condition_variable startCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
};
AEncBufferSignal *signal_ = new AEncBufferSignal();

bool isFirstFrame = true;
int32_t qpAverage = 20;
double mseValue = 0.0;
// 配置视频帧宽度（必须）
int32_t width = 1080;
// 配置视频帧高度（必须）
int32_t height = 1920;
int32_t widthStride = 0;
int32_t heightStride = 0;

static void OnError(OH_AVScreenCapture *capture, int32_t errorCode, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::OnError and errorCode::%{public}d", errorCode);
    (void)capture;
    (void)errorCode;
    (void)userData;
};
static void OnStateChange(struct OH_AVScreenCapture *capture, OH_AVScreenCaptureStateCode stateCode, void *userData) {
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

static void OnBufferAvailable(OH_AVScreenCapture *capture, OH_AVBuffer *buffer, OH_AVScreenCaptureBufferType bufferType,
                              int64_t timestamp, void *userData) {
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
        // 每帧样点数
        int32_t DEFAULT_SAMPLERATE = 48000;
        int32_t TIME_PER_FRAME = 0.02;
        int32_t SAMPLES_PER_FRAME = DEFAULT_SAMPLERATE * TIME_PER_FRAME;
        // 声道数，对于amr编码声道数只支持单声道的音频输入
        int32_t DEFAULT_CHANNEL_COUNT = 2;
        // 每帧输入数据的长度，声道数 * 每帧样点数 * 每个样点的字节数（以采样格式SAMPLE_S16LE为例）
        int32_t INPUT_FRAME_BYTES = DEFAULT_CHANNEL_COUNT * SAMPLES_PER_FRAME * sizeof(short);

        AEncBufferSignal *signal = static_cast<AEncBufferSignal *>(userData);

        std::unique_lock<std::mutex> lock(signal->inMutex_);
        signal->inCond_.wait(lock, [&]() { return (signal->inQueue_.size() > 0); });
        OH_LOG_DEBUG(LOG_APP, "wangz::OnBufferAvailable::debug::1");
        uint32_t index = signal->inQueue_.front();
        auto micBuffer = signal->inBufferQueue_.front();
        OH_LOG_DEBUG(LOG_APP, "wangz::OnBufferAvailable::debug::2");
        memcpy(OH_AVBuffer_GetAddr(micBuffer), buffer, bufferLen);
        OH_AVCodecBufferAttr attr = {0};
        attr.size = bufferLen;
        OH_AVErrCode ret = OH_AVBuffer_SetBufferAttr(micBuffer, &attr);
        if (ret != AV_ERR_OK) {
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_AVBuffer_SetBufferAttr::%{public}d", ret);
        }
        // 送入编码输入队列进行编码, index为对应队列下标
        ret = OH_AudioCodec_PushInputBuffer(micCodec, index);
        if (ret != AV_ERR_OK) {
            OH_LOG_DEBUG(LOG_APP, "wangz::OH_AudioCodec_PushInputBuffer::%{public}d", ret);
        }
        signal->inQueue_.pop();
        signal->inBufferQueue_.pop();
    }
};

void CreateAndInitWithSurfaceMode(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::CreateAndInitWithSurfaceMode");
    capture = OH_AVScreenCapture_Create();

    // 可选 配置录屏旋转，此接口在感知到手机屏幕旋转时调用，如果手机的屏幕实际上没有发生旋转，调用接口是无效的。
    OH_AVScreenCapture_SetCanvasRotation(capture, true);
    // 可选 [过滤音频]
    OH_AVScreenCapture_ContentFilter *contentFilter = OH_AVScreenCapture_CreateContentFilter();
    // 添加过滤通知音
    OH_AVScreenCapture_ContentFilter_AddAudioContent(contentFilter, OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO);
    // 排除过滤器
    OH_AVScreenCapture_ExcludeContent(capture, contentFilter);

    // 初始化录屏，传入配置信息OH_AVScreenRecorderConfig
    OH_AudioCaptureInfo miccapinfo = {.audioSampleRate = 48000, .audioChannels = 2, .audioSource = OH_MIC};
    OH_AudioCaptureInfo innerCapInfo = {.audioSampleRate = 48000, .audioChannels = 2, .audioSource = OH_ALL_PLAYBACK};
    OH_AudioEncInfo audioEncInfo = {.audioBitrate = 48000, .audioCodecformat = OH_AAC_LC};
    OH_VideoCaptureInfo videoCapInfo = {
        .videoFrameWidth = 720, .videoFrameHeight = 1080, .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
    OH_VideoEncInfo videoEncInfo = {.videoCodec = OH_H264, .videoBitrate = 2000000, .videoFrameRate = 30};
    OH_AudioInfo audioInfo = {.micCapInfo = miccapinfo, .innerCapInfo = innerCapInfo, .audioEncInfo = audioEncInfo};
    OH_VideoInfo videoInfo = {.videoCapInfo = videoCapInfo, .videoEncInfo = videoEncInfo};
    OH_AVScreenCaptureConfig config = {
        .captureMode = OH_CAPTURE_HOME_SCREEN,
        .dataType = OH_ORIGINAL_STREAM,
        .audioInfo = audioInfo,
        .videoInfo = videoInfo,
    };
    OH_AVScreenCapture_SetErrorCallback(capture, OnError, signal_);
    OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, signal_);
    OH_AVScreenCapture_SetDataCallback(capture, OnBufferAvailable, signal_);

    // 初始化录屏参数，传入配置信息OH_AVScreenRecorderConfig
    //     OH_RecorderInfo recorderInfo;
    //     const std::string SCREEN_CAPTURE_ROOT = "/data/storage/el2/base/haps/entry/files/";
    //     int32_t outputFd = open((SCREEN_CAPTURE_ROOT + "screen01.mp4").c_str(), O_RDWR | O_CREAT, 0777);
    //     std::string fileUrl = "fd://" + std::to_string(outputFd);
    //     recorderInfo.url = const_cast<char *>(fileUrl.c_str());
    //     recorderInfo.fileFormat = OH_ContainerFormatType::CFT_MPEG_4;
    //     config.recorderInfo = recorderInfo;

    int32_t retInit = OH_AVScreenCapture_Init(capture, config);
    OH_LOG_DEBUG(LOG_APP, "wangz::END::OH_AVScreenCapture_Init::%{public}d", retInit);
}

static void ConfigEncoderWithSurfaceMode(void) {
    // 配置视频像素格式（必须）
    constexpr OH_AVPixelFormat DEFAULT_PIXELFORMAT = AV_PIXEL_FORMAT_NV12;
    // 配置视频帧速率
    double frameRate = 30.0;
    // 配置视频YUV值范围标志
    bool rangeFlag = false;
    // 配置视频原色
    int32_t primary = static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT709);
    // 配置传输特性
    int32_t transfer = static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709);
    // 配置最大矩阵系数
    int32_t matrix = static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_IDENTITY);
    // 配置编码Profile
    int32_t profile = static_cast<int32_t>(OH_AVCProfile::AVC_PROFILE_BASELINE);
    // 配置编码比特率模式
    int32_t rateMode = static_cast<int32_t>(OH_VideoEncodeBitrateMode::CBR);
    // 配置关键帧的间隔，单位为毫秒
    int32_t iFrameInterval = 60000;
    // 配置比特率
    int64_t bitRate = 1 * 1024 * 1024;
    //     int64_t bitRate = 300000;
    // 配置编码质量
    int64_t quality = 0;

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, width);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, height);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIXELFORMAT);

    //     OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);
    //     OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, rangeFlag);
    //     OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, primary);
    //     OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, transfer);
    //     OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, matrix);
    //     OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, iFrameInterval);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, profile);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, rateMode);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitRate);
    // 只有当OH_MD_KEY_BITRATE = CQ时，才需要配置OH_MD_KEY_QUALITY
    if (rateMode == static_cast<int32_t>(OH_VideoEncodeBitrateMode::CQ)) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, quality);
    }
    int32_t ret = OH_VideoEncoder_Configure(codec, format);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Configure::%{public}d", ret);
    }
    OH_AVFormat_Destroy(format);
}

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode::OnError");
    (void)codec;
    (void)errorCode;
    (void)userData;
}

static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode::OnStreamChanged");
    (void)codec;
    (void)format;
    (void)userData;
}

// Surface模式
static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode::OnNeedInputBuffer");
    (void)userData;
    (void)index;
    (void)buffer;
}

// Surface模式
static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    //     OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode::OnNewOutputBuffer");
    //         int32_t qpAverage = 20;
    //         double mseValue = 0.0;
    // 完成帧buffer对应的index，送入outIndexQueue队列
    // 完成帧的数据buffer送入outBufferQueue队列
    // 获取视频帧的平均量化参数,平方误差
    OH_AVFormat *format = OH_AVBuffer_GetParameter(buffer);
    //         OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_AVERAGE, qpAverage);
    //         OH_AVFormat_GetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_MSE, mseValue);
    OH_AVFormat_Destroy(format);
    // 数据处理，请参考:
    // 释放编码帧
    // 获取编码后信息
    OH_AVCodecBufferAttr info;
    int32_t ret = OH_AVBuffer_GetBufferAttr(buffer, &info);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode::OnNewOutputBuffer::OH_AVBuffer_GetBufferAttr::%{public}d",
                     ret);
    }
    // 将编码完成帧数据buffer写入到对应输出文件中
    // 写入数据到文件
    if (outputFile->is_open()) {
        outputFile->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), info.size);
        outputFile->flush();
    } else {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnNewOutputBuffer::outputFile::error");
    }
    // 检查是否写入成功
    if (!outputFile->good()) {
        // 处理错误情况
        OH_LOG_DEBUG(LOG_APP, "wangz::OnNewOutputBuffer::outputFile::write error");
    }

    //     if (std::filesystem::exists("/data/storage/el2/base/haps/entry/files/screen01.mp4")) {
    //         OH_LOG_DEBUG(LOG_APP, "wangz::File exists and size is: %{public}d ",
    //                      std::filesystem::file_size("/data/storage/el2/base/haps/entry/files/screen01.mp4"));
    //     } else {
    //         OH_LOG_DEBUG(LOG_APP, "wangz::File does not exist.");
    //     }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    void *bufferData = std::malloc(info.size);
    memcpy(bufferData, OH_AVBuffer_GetAddr(buffer), info.size);
    OH_LOG_DEBUG(LOG_APP, "wangz::OutputFunc index: %{public}d, size: %{public}d, pts: %{public}d, flags: %{public}d ",
                 index, info.size, timestamp / 1000, info.flags);
    //     encodeScreenResultCallback(dataToPass);
    // 释放已完成写入的数据，index为对应输出队列下标
    ret = OH_VideoEncoder_FreeOutputBuffer(codec, index);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnNewOutputBuffer::OH_VideoEncoder_FreeOutputBuffer::%{public}d", ret);
    }
}


// OH_AVCodecOnStreamChanged回调函数的实现
static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::OnOutputFormatChanged::");
    (void)codec;
    (void)format;
    (void)userData;
}
// OH_AVCodecOnNeedInputBuffer回调函数的实现
static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::OnInputBufferAvailable::START");
    (void)codec;
    // 编码输入码流送入InputBuffer队列
    AEncBufferSignal *signal = static_cast<AEncBufferSignal *>(userData);
    std::unique_lock<std::mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
    OH_LOG_DEBUG(LOG_APP, "wangz::OnInputBufferAvailable::END");
}
// OH_AVCodecOnNewOutputBuffer回调函数的实现
static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData) {
    OH_LOG_DEBUG(LOG_APP, "wangz::OnOutput::START");
    (void)codec;
    // 将对应输出buffer的index送入OutputQueue_队列
    // 将对应编码完成的数据data送入outBuffer队列
    AEncBufferSignal *signal = static_cast<AEncBufferSignal *>(userData);
    std::unique_lock<std::mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
    OH_LOG_DEBUG(LOG_APP, "wangz::OnOutput::END");
}


int32_t ret;
// 配置音频采样率（必须）
constexpr uint32_t DEFAULT_SAMPLERATE = 44100;
// 配置音频码率（必须）
constexpr uint64_t DEFAULT_BITRATE = 32000;
// 配置音频声道数（必须）
constexpr uint32_t DEFAULT_CHANNEL_COUNT = 2;
// 配置音频声道类型（必须）
constexpr OH_AudioChannelLayout CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_STEREO;
// 配置音频位深（必须）
constexpr OH_BitsPerSample SAMPLE_FORMAT = OH_BitsPerSample::SAMPLE_S16LE;
// 配置音频compliance level (默认值0，取值范围-2~2)
constexpr int32_t COMPLIANCE_LEVEL = 0;
// 配置音频精度（必须） SAMPLE_S16LE
constexpr OH_BitsPerSample BITS_PER_CODED_SAMPLE = OH_BitsPerSample::SAMPLE_S16LE;
// 每20ms一帧音频数据
constexpr float TIME_PER_FRAME = 0.02;
// 配置最大输入长度, 每帧音频数据的大小（可选）
constexpr uint32_t DEFAULT_MAX_INPUT_SIZE =
    DEFAULT_SAMPLERATE * TIME_PER_FRAME * DEFAULT_CHANNEL_COUNT * sizeof(short); // aac


void StartMic(void) {
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);
    const char *name = OH_AVCapability_GetName(capability);
    micCodec = OH_AudioCodec_CreateByName(name);

    //     signal_ = new AEncBufferSignal();
    OH_AVCodecCallback cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    // 配置异步回调
    int32_t ret = OH_AudioCodec_RegisterCallback(micCodec, cb_, signal_);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_AudioCodec_RegisterCallback::%{public}d", ret);
    }

    OH_AVFormat *format = OH_AVFormat_Create();
    // 写入format
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, DEFAULT_SAMPLERATE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, DEFAULT_MAX_INPUT_SIZE);
    // 配置编码器
    ret = OH_AudioCodec_Configure(micCodec, format);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_AudioCodec_Configure::%{public}d", ret);
    }

    ret = OH_AudioCodec_Prepare(micCodec);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_AudioCodec_Prepare::%{public}d", ret);
    }

    // 开始编码
    ret = OH_AudioCodec_Start(micCodec);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_AudioCodec_Start::%{public}d", ret);
    }

    std::unique_ptr<std::thread> micEncOutputThread_ = std::make_unique<std::thread>([&]() {
        while (true) {
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::1");
            std::unique_lock<std::mutex> lock(signal_->outMutex_);
            signal_->outCond_.wait(lock, [&]() { return (signal_->outQueue_.size() > 0); });
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::2");
            uint32_t index = signal_->outQueue_.front();
            OH_AVBuffer *avBuffer = signal_->outBufferQueue_.front();
            OH_AVCodecBufferAttr attr = {0};
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::3");
            if (OH_AVBuffer_GetBufferAttr(avBuffer, &attr) != AV_ERR_OK) {
                OH_LOG_ERROR(LOG_APP, "wangz::OutputFunc GetBufferAttr error");
            }
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::4");
            //         size += attr.size;
            void *buffer_data = std::malloc(attr.size);
            memcpy(buffer_data, OH_AVBuffer_GetAddr(avBuffer), attr.size);
            OH_LOG_INFO(LOG_APP, "wangz::MIC index: %{public}d, flags: %{public}d size: %{public}d", index, attr.flags,
                        attr.size);
            //         PLAudioFrame *dataToPass = new PLAudioFrame{buffer_data, index, attr.size, attr.pts / 1000,
            //         attr.flags}; encodeResultCallback(dataToPass);
            if (OH_AudioCodec_FreeOutputBuffer(micCodec, index) != AV_ERR_OK) {
                OH_LOG_ERROR(LOG_APP, "wangz::Fatal: FreeOutputData fail");
                break;
            }
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::5");
            signal_->outBufferQueue_.pop();
            signal_->outQueue_.pop();
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::6");
            if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
                OH_LOG_ERROR(LOG_APP, "wangz::decode eos");
                //             isRunning_.store(false);
                signal_->startCond_.notify_all();
            }
            OH_LOG_DEBUG(LOG_APP, "wangz::out::debug::7");
        }
    });
    micEncOutputThread_->detach();
}

void StartWithSurfaceMode(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StartWithSurfaceMode");
    // 通过 MIME TYPE 创建编码器，系统会根据MIME创建最合适的编码器。
    OH_AVCapability *capability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true);
    const char *codecName = OH_AVCapability_GetName(capability);
    codec = OH_VideoEncoder_CreateByName(codecName);
    OH_AVCodecCallback cb = {OnError, OnStreamChanged, OnNeedInputBuffer, OnNewOutputBuffer};
    int32_t ret = OH_VideoEncoder_RegisterCallback(codec, cb, nullptr); // NULL:用户特定数据userData为空
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::END::OH_VideoEncoder_RegisterCallback::%{public}d", ret);
    }

    ConfigEncoderWithSurfaceMode();

    OHNativeWindow *nativeWindow;
    // 从视频编码器获取输入Surface
    OH_AVErrCode retGetSurface = OH_VideoEncoder_GetSurface(codec, &nativeWindow);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_GetSurface::%{public}d", retGetSurface);
    int32_t retPrepare = OH_VideoEncoder_Prepare(codec);
    OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_Prepare::%{public}d", retPrepare);

    // 配置待编码文件路径
    outputFile->open(outputFilePath.data(), std::ios::out | std::ios::binary | std::ios::trunc);

    if (!outputFile->is_open()) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnNewOutputBuffer::outputFile::error");
    }
    if (!outputFile->good()) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OnNewOutputBuffer::outputFile::write error");
    }
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

void StopAndReleaseWithSurfaceMode(void) {
    OH_LOG_DEBUG(LOG_APP, "wangz::StopAndReleaseWithSurfaceMode");

    int32_t ret;
    ret = OH_VideoEncoder_NotifyEndOfStream(codec);
    if (ret != AV_ERR_OK) {
        OH_LOG_DEBUG(LOG_APP, "wangz::OH_VideoEncoder_NotifyEndOfStream::failed");
    }

    // 刷新编码器videoEnc
    ret = OH_VideoEncoder_Flush(codec);
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

void Test(void) {
    if (!isRunning) {
        {
            CreateAndInitWithSurfaceMode();
            StartWithSurfaceMode();
            StartMic();
            isRunning = true;
        }
    } else {
        {
            StopAndReleaseWithSurfaceMode();
            isRunning = false;
        }
    }
}

static napi_value ScreenTest(napi_env env, napi_callback_info info) {
    Test();
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
    //     NativeXComponentSample::PluginManager::GetInstance()->Export(env, exports);
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
