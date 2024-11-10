#ifndef PLAYER_H
#define PLAYER_H
#include <thread>
#include <functional>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <pthread.h>
#include <stdlib.h>
#include <SDL.h>
#include <pthread.h>
}
#include <queue.h>
#include <QString>
#include <openglwidget.h>

enum State{
    NONE = 0,// 默认值为NONE
    PLAYER,//正在播放
    STOP,// 暂停播放
    SEEK,// 视频跳转播放
    FINISH,// 完成视频播放
    EXIT// 视频播放退出
};

class Player
{
friend void fill_audio_pcm(void *data, uint8_t *stream, int len);

public:
    Player();
    ~Player();
    // 开始播放
    bool Begin();
    // 暂停播放
    void Stop();
    // 恢复暂停状态到播放状态
    void Play();
    // 退出播放
    void Exit();
    // seek到指定位置播放
    void Seek(int Pos);
    // 设置文件路径(在这里也完成了上下文和解码器相关的初始化
    void SetFilePath(QString path);

    void setUpdateCallback(const std::function<void()>& callback) {
        updateCallback = callback;
    }

    void setRenderCallback(const std::function<void(H264YUV_Frame*)>& callback) {
        RenderCallback = callback;
    }
    int TotalTime = 0;
private:
    void SDLAudioInit();

    void InitDecode();

    void DeInitDecode();

    int VideoThread();

    int AudioThread();

    int ReadThread();

    void videoDelay(AVFrame* frame);

    void audioDelay(AVFrame* frame);

    void ShowPicture(AVFrame* frame);

    void PlayAudio(AVFrame* frame);

    // 记录当前播放器的状态
    State state = NONE;

    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *aCodecCtx = NULL;
    AVCodecContext *vCodecCtx = NULL;

    // 视频索引
    int videoIndex = -1;
    // 音频索引
    int audioIndex = -1;

    SwrContext* swrCtx = NULL;
    SwsContext* swsCtx = NULL;

    //三个线程
    // read线程
    std::thread *Read = NULL;
    // video线程
    std::thread *Video = NULL;
    // audio线程
    std::thread *Audio = NULL;

    // 视频包队列
    Queue VideoQueue;
    // 音频包队列
    Queue AudioQueue;

    // 当前视频帧的时间
    double CurrentVideoTimeStamp = 0.0f;
    // 上一帧的视频时间
    double LastVideoTimeStamp = 0.0f;
    // 音频时间戳
    double AudioTimeStamp = 0.0f;
    // 上一帧的音频时间戳
    double LastAudioTimeStamp = 0.0f;
    QString filePath = "";
    // 更新进度条和当前的播放时间
    std::function<void()> updateCallback;
    std::function<void(H264YUV_Frame*)> RenderCallback;

    //SDL相关变量
    // 播放器的宽
    int width = 1280;
    // 播放器的高
    int height = 720;

    uint32_t pixformat = SDL_PIXELFORMAT_IYUV;  // YUV420P，即是SDL_PIXELFORMAT_IYUV

    SDL_AudioSpec spec;

    bool doSeek = false;

    pthread_mutex_t Mut;
    pthread_cond_t Cond;

    //用于seek后的音视频同步的信号量
    pthread_mutex_t timeMut;
    pthread_cond_t timeCond;
};

#endif // PLAYER_H
