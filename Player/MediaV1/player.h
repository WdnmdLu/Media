#ifndef PLAYER_H
#define PLAYER_H
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
public:
    Player();
    // 开始播放
    void Begin();
    // 暂停播放
    void Stop();
    // 退出播放
    void Exit();
    // seek到指定位置播放
    void Seek();

private:
    void VideoThread();

    void AudioThread();

    void ReadThread();

    // 初始化解码器
    void DecodeInit();
    // 释放解码器
    void DecodeDeInit();
    // 记录当前播放器的状态
    State state = NONE;

    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *aCodecCtx = NULL;
    AVCodecContext *vCodecCtx = NULL;

    // 视频索引
    int videoIndex;
    // 音频索引
    int audioIndex;

    //三个线程
    // read线程
    pthread_t Read;
    // video线程
    pthread_t Video;
    // Audio线程
    pthread_t Audio;
    // 视频包队列
    Queue VideoQueue;
    // 音频包队列
    Queue AudioQueue;

    //SDL相关变量
    int width = 1280;
    int height = 720;

};

#endif // PLAYER_H
