#include "main.h"
pktQueue *audioQueue;
pktQueue *videoQueue;
static int Exit = 0;
static int audioExit = 0;
static int videoExit = 0;

static char err_buf[128] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}

#define PCM_BUFFER_SIZE (1024*2*2*2)
// 音频PCM数据缓存
static Uint8 *s_audio_buf = NULL;
// 目前读取的位置
static Uint8 *s_audio_pos = NULL;
// 缓存结束位置
static Uint8 *s_audio_end = NULL;

static int videoThread(void *data){
    AVCodecContext *CodecCtx = (AVCodecContext*)data;

    AVFrame *frame = av_frame_alloc();
    int ret;

    // SDL
    SDL_Event event;                            // 事件
    SDL_Rect rect;                              // 矩形
    SDL_Window *window = NULL;                  // 窗口
    SDL_Renderer *renderer = NULL;              // 渲染
    SDL_Texture *texture = NULL;                // 纹理
    uint32_t pixformat = SDL_PIXELFORMAT_IYUV;            // YUV420P，即是SDL_PIXELFORMAT_IYUV

    //创建窗口
    window = SDL_CreateWindow("Simplest YUV Player",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           CodecCtx->width, CodecCtx->height,
                           SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);


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
                                CodecCtx->width,
                                CodecCtx->height);

    while(Exit == 0 || videoQueue->GetSize() > 0){
        if(videoQueue->GetSize() > 0){
            avcodec_send_packet(CodecCtx,(AVPacket*)videoQueue->Back());
            while((ret = avcodec_receive_frame(CodecCtx,frame)) >= 0){
                SDL_Delay(20);
                printf("Read one frame\n");
                //开始播放这一帧的图像数据
                SDL_UpdateYUVTexture(texture, NULL,
                                     frame->data[0], frame->linesize[0],
                                     frame->data[1], frame->linesize[1],
                                     frame->data[2], frame->linesize[2]);

                rect.x = 0;
                rect.y = 0;

                rect.w = CodecCtx->width;
                rect.h = CodecCtx->height;
                SDL_RenderClear(renderer);
                // 将纹理的数据拷贝给渲染器
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                // 显示
                SDL_RenderPresent(renderer);
            }
            videoQueue->Pop();
        }
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT){
            Exit = 1;
            break;
        }
    }

    videoExit = -1;
    return 0;
}

// 音频回调函数，用于填充音频数据到SDL的音频缓冲区
static void Fill_audio_pcm(void *data, uint8_t *stream, int len) {
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
    printf("Player one frame\n");
    // 更新缓存中的当前位置
    s_audio_pos += len;
}

int AudioThread(void *data){
    AVCodecContext *codecCtx = (AVCodecContext*)data;
    AVFrame *frame = av_frame_alloc();
    int data_size,i,ret;
    SDL_AudioSpec spec;
    //SDL音频操作初始化
    // 配置SDL音频规格
    spec.freq = 44100;              // 采样率 44100 Hz
    spec.format = AUDIO_F32SYS;     // 32-bit 浮点数格式
    spec.channels = 1;              // 单声道
    spec.silence = 0;               // 无需设置静音值（不适用浮点格式）
    spec.samples = PCM_BUFFER_SIZE / 8;        // 每次音频回调处理的样本数量
    spec.callback = Fill_audio_pcm; // 音频数据填充回调函数
    spec.userdata = NULL;           // 回调函数数据（未使用）
    s_audio_buf = (uint8_t*)malloc(PCM_BUFFER_SIZE);
    // 打开SDL音频设备
    if (SDL_OpenAudio(&spec, NULL)) {
        fprintf(stderr, "Failed to open audio device\n");
        SDL_Quit();
        free(s_audio_buf);
        audioExit = -1;
        return 0;
    }
    // 开始播放音频
    SDL_PauseAudio(0);

    while(Exit == 0 || audioQueue->GetSize() > 0){
        if(audioQueue->GetSize() > 0){
            avcodec_send_packet(codecCtx,(AVPacket*)audioQueue->Back());
            while((ret = avcodec_receive_frame(codecCtx,frame)) >= 0){
                //将这一帧数据写入到缓存中
                data_size = av_get_bytes_per_sample(codecCtx->sample_fmt);
                if(data_size < 0){
                    audioExit = -1;
                    return -1;
                }
                for(i = 0; i<frame->nb_samples;i++){
                    //fwrite(frame->data[ret] + data_size*i,1,data_size,out);
                    memcpy(s_audio_buf+i*data_size,frame->data[ret] + data_size*i,data_size);
                }
                s_audio_end = s_audio_buf + data_size*frame->nb_samples;
                s_audio_pos = s_audio_buf;
                // 等待直到所有数据被播放
                while (s_audio_pos < s_audio_end) {
                    SDL_Delay(10); // 暂停10毫秒，以防CPU过度使用
                }
            }
            audioQueue->Pop();
        }
    }
    audioExit = -1;
}

void MediaPlayer02(const char *filename){
    audioQueue = new pktQueue();
    videoQueue = new pktQueue();
    AVFormatContext *fmt_ctx = NULL;

    AVCodecContext *aCodecCtx = NULL;
    const AVCodec *aCodec = NULL;

    AVCodecContext *vCodecCtx = NULL;
    const AVCodec *vCodec = NULL;

    int8_t audioStream = -1,videoStream = -1;
    uint8_t i;
    // 打开输入文件
    if(avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0){
        printf("Open file failed\n");
        return;
    }
        printf("Begin find stream info\n");
    // 检查是否是媒体文件
    if(avformat_find_stream_info(fmt_ctx, NULL) < 0){
        av_log(NULL,AV_LOG_ERROR,"Failed to find stream");
        avformat_close_input(&fmt_ctx);
        return;
    }

    for(i = 0;i<fmt_ctx->nb_streams;i++){
        if(fmt_ctx->streams[i]->index == AVMEDIA_TYPE_AUDIO){
            audioStream = i;
        }
        else if(fmt_ctx->streams[i]->index == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
        }
    }
    if(audioStream == -1 || videoStream == -1){
        avformat_close_input(&fmt_ctx);
        return;
    }

    aCodec = avcodec_find_decoder(fmt_ctx->streams[audioStream]->codecpar->codec_id);
    aCodecCtx = avcodec_alloc_context3(aCodec);
    avcodec_parameters_to_context(aCodecCtx,fmt_ctx->streams[audioStream]->codecpar);
    avcodec_open2(aCodecCtx,aCodec,NULL);

    vCodec = avcodec_find_decoder(fmt_ctx->streams[videoStream]->codecpar->codec_id);
    vCodecCtx = avcodec_alloc_context3(vCodec);
    avcodec_parameters_to_context(vCodecCtx,fmt_ctx->streams[videoStream]->codecpar);
    avcodec_open2(vCodecCtx,vCodec,NULL);

    SDL_CreateThread(AudioThread,"AudioThread",aCodecCtx);

    SDL_CreateThread(videoThread,"VideoThread",vCodecCtx);
    while(1){
        AVPacket *pkt = (AVPacket*)malloc(sizeof(AVPacket));
        if(av_read_frame(fmt_ctx,pkt) >=0 ){
            if(pkt->stream_index == audioStream){
                audioQueue->Push(pkt);
            }
            else if(pkt->stream_index == videoStream){
                videoQueue->Push(pkt);
            }
        }
        if(Exit == 1){
            break;
        }
    }
    while(audioExit == 0 || videoExit == 0){
        SDL_Delay(20);
    }
    avformat_close_input(&fmt_ctx);
    return;
}
