#ifndef MAIN_H
#define MAIN_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
extern "C"{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/mathematics.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <pthread.h>
}

#include <SDL.h>
#include <condition_variable>
#include <mutex>


// 解析MP4文件生成对应的aac码流并输出到文件中
int adts_header(char * const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels);
// 解码MP4生成AAC
void ADTS_Header();

// 解码MP4文件生成对应的h264码流并输出到文件中
void Extract_h264(const char *input_filename, const char *output_filename);

void DecodeMP4(const char *filename);

void Dexumer();


void PCMPlayer(const char *filename);
// 解码MP4生成YUV
void DecodeMP4toYUV(const char *filename);
// 解码MP4生成PCM
void DecodeMP4toPCM(const char *filename);

void YUV_Player();

void VideoPlayer(const char *filename);

void AudioPlayer(const char *filename);

void Test(const char *filename);

void MediaPlayer(const char *filename);

void MediaPlayer02(const char *filrname);


struct Node {
    void *data;
    // 指向下一个元素
    struct Node *next;
    // 指向上一个元素
    struct Node *pre;
    Node() : data(nullptr), next(nullptr){}
};

class pktQueue;
#endif // MAIN_H
