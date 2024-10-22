#include "sdlshow.h"
#include "SDL.h"

// 音频PCM数据缓存
static Uint8 *s_audio_buf = NULL;
// 目前读取的位置
static Uint8 *s_audio_pos = NULL;
// 缓存结束位置
static Uint8 *s_audio_end = NULL;

void fill_audio_pcm(void *data, uint8_t *stream, int len){
    // 清空音频流
    SDL_memset(stream, 0, len);
    // 检查是否已经没有更多数据
    if (s_audio_pos >= s_audio_end) {
        return;
    }

    // 计算实际可用的数据长度
    int remain_buffer_len = s_audio_end - s_audio_pos;
    len = (len < remain_buffer_len) ? len : remain_buffer_len;

    // 将数据从缓存填充到音频流
    SDL_MixAudio(stream, s_audio_pos, len, SDL_MIX_MAXVOLUME / 8);
    // 更新缓存中的当前位置
    s_audio_pos += len;
}

SdlShow::SdlShow(int w,int h,void* win){
    winWidth = w;
    winHeight = h;
    //音频和视频初始化
    //初始化opengl和sdl，用于后续的视频渲染以及音频播放
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);

    spec.freq = 44100;             // 采样率 44100 Hz
    spec.format = AUDIO_F32SYS;    // 32-bit 浮点数格式
    spec.channels = 2;             // 单声道
    spec.silence = 0;              // 无需设置静音值（不适用浮点格式）
    spec.samples = 1024; // 每次音频回调处理的样本数量
    spec.callback = fill_audio_pcm; // 音频数据填充回调函数
    spec.userdata = NULL;          // 回调函数数据（未使用）

    SDL_OpenAudio(&spec, NULL);

    window = SDL_CreateWindowFrom(win);
    if(!window)
    {
        fprintf(stderr, "SDL: could not create window, err:%s\n",SDL_GetError());

    }
    // 基于窗口创建渲染器   这个渲染器是在这个窗口内进行渲染的
    renderer = SDL_CreateRenderer(window, -1, 0);
    // 基于渲染器创建纹理
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                this->winWidth,
                                this->winHeight);
    s_audio_buf = (uint8_t*)malloc(1024*2*2*2);
}
SdlShow::~SdlShow(){

}

// 播放一帧音频数据
void SdlShow::PlayerAudioFrame(AVFrame *frame){
    int i,j,len,data_size,ch;
    //将这一帧数据写入到缓存中
    data_size = av_get_bytes_per_sample((AVSampleFormat)frame->format);
    if(data_size < 0){
        return;
    }
    s_audio_end = s_audio_buf;
    len = 0;
    // frame->nb_samples 这一帧音频数据有nb_samples个样本  通常这个值为1024
    for (i = 0,j = 0; i < frame->nb_samples; i++)
    {
        for (ch = 0; ch < frame->ch_layout.nb_channels; ch++)  // 交错的方式写入, 大部分float的格式输出
        {
            memcpy(s_audio_buf + data_size * j, frame->extended_data[ch] + data_size * i, data_size);
            len += data_size;
            j++;
        }
    }
    s_audio_end = s_audio_buf + len;
    s_audio_pos = s_audio_buf;
    // 等待直到所有数据被播放
    while (s_audio_pos < s_audio_end) {
        SDL_Delay(10); // 暂停10毫秒，以防CPU过度使用
    }
}

// 解码一帧frame并进行显示
void SdlShow::ShowPicture(AVFrame *frame){
    SDL_UpdateYUVTexture(texture, NULL,
                         frame->data[0], frame->linesize[0],
                         frame->data[1], frame->linesize[1],
                         frame->data[2], frame->linesize[2]);

    rect.x = 0;
    rect.y = 0;

    rect.w = frame->width;
    rect.h = frame->height;
    SDL_RenderClear(renderer);
    // 将纹理的数据拷贝给渲染器
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    // 显示
    SDL_RenderPresent(renderer);
}
