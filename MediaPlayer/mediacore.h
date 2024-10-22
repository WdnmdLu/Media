#ifndef MEDIACORE_H
#define MEDIACORE_H
#include <string>
#include <sdlshow.h>
#include <functional>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <pthread.h>
#include <stdlib.h>
#include <SDL.h>
}

class Queue{
public:
    Queue(int Cap = 2);
    ~Queue();
    //放入
    void Push(void *data);
    // 队头的数据出队
    void Pop();
    // 获取到队头的数据
    void *Front();
    // 进行二倍扩容
    void expand();

    int GetSize();

    int GetCap();
    // 释放掉队列中的所有数据，将队列清空
    void ClearQueue();

private:
    struct Node{
        struct Node *next;
        // 通过void*用户可以指定任意的数据类型，可以是基本数据类型，int，float或者特殊结构体，AVPacket，AVFrame等等
        void* data;
    };
private:
    // 队列当前的数据量大小
    int size;
    // 队列的容量
    int Capacity;
    // 队头指针，用于表示整个队列
    Node *head;
    // 最新入队的数据
    Node *curr;
    // 指向最后一个位置的节点
    Node *tail;

    pthread_mutex_t Mut;
    pthread_cond_t Cond;
};

void* DexumerThread(void* p);
// 视频解码线程
void* VideoThread(void *p);
// 音频解码线程
void* AudioThread(void *p);

enum MediaStatus{
    NONE = 0,
    PLAYER,//正在播放
    STOP,// 暂停播放
    SEEK,// 视频跳转播放
    FINISH,// 完成视频播放
    EXIT// 视频播放退出
};

class MediaCore
{
        // 解复用线程
friend void* DexumerThread(void* p);
        // 视频解码线程
friend void* VideoThread(void *p);
        // 音频解码线程
friend void* AudioThread(void *p);

friend void fill_audio_pcm(void *data, uint8_t *stream, int len);
public:
    MediaCore(int w,int h,void* win);
    ~MediaCore();

    int Init();

    void SetPath(const char *Path);

    void unInit();

    void StartPlayer();

    void StopPlayer();

    void ExitPlayer();
    // Seek到指定的位置进行播放
    void Seek(int Times);

    void setUpdateCallback(const std::function<void()>& callback) {
        updateCallback = callback;
    }

    std::string durationStr;

    int64_t total_seconds = 0;
private:
    std::function<void()> updateCallback;

    float GetVideoPTS(uint64_t pts);

    float GetAudioPTS(uint64_t pts);

    void VideoDelay(int64_t pts);

    void AudioDelay(int64_t pts);

    std::string filePath;
    AVFormatContext *fmt_ctx;
    int videoIndex;
    int audioIndex;

    AVCodecContext* videoCodecCtx;
    AVCodecContext* audioCodecCtx;

    MediaStatus sts = NONE;

    Queue *videoQueue;
    Queue *audioQueue;
    // 用于剪切转换视频文件，例如1920*1080的视频要转化成720*640的视频才能进行播放
    SwsContext *videoSwsCtx;
    // 重采样结构体
    SwrContext *audioSwrCtx;

    int winWidth;
    int winHeight;

    AVFrame *videoFrame;
    AVFrame *audioFrame;

    //采样率
    int sampleRate = 44100;//每秒采样44100次
    //采样格式
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_S16;//16bit采样
    //通道数
    AVChannelLayout *channelLayout = NULL;//双通道立体声
    //音量
    int volumeRatio = 1;

    // 音频当前播放的时间戳
    float audioTimeStamp = 0.0f;
    float audioLastTimeStamp = 0.0f;

    // 视频当前播放的时间戳
    float videoTimeStamp = 0.0f;
    float videoLastTimeStamp = 0.0f;

    SdlShow *sdl;

    pthread_cond_t Cond1;
    pthread_mutex_t Mut1;

    pthread_cond_t Cond2;
    pthread_mutex_t Mut2;

    pthread_cond_t Cond3;
    pthread_mutex_t Mut3;
};

#endif // MEDIACORE_H
