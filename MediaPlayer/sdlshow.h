#ifndef SDLSHOW_H
#define SDLSHOW_H

#include "SDL.h"
extern "C"{
#include <libavformat/avformat.h>
}
class SdlShow
{
public:
    SdlShow(int w,int h,void* win);
    ~SdlShow();
    // 播放一帧音频数据
    void PlayerAudioFrame(AVFrame *frame);

    // 解码一帧frame并进行显示
    void ShowPicture(AVFrame *frame);
private:

    int winWidth;
    int winHeight;

    // SDL
    SDL_Event event;                            // 事件
    SDL_Rect rect;                              // 矩形
    SDL_Window *window = NULL;                  // 窗口
    SDL_Renderer *renderer = NULL;              // 渲染
    SDL_Texture *texture = NULL;                // 纹理
    uint32_t pixformat = SDL_PIXELFORMAT_IYUV;  // YUV420P，即是SDL_PIXELFORMAT_IYUV

    SDL_AudioSpec spec;
};

#endif // SDLSHOW_H
