#include "Decoder.h"

static char err_buf[128] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}

//自定义消息类型
#define REFRESH_EVENT   (SDL_USEREVENT + 1)     // 请求画面刷新事件
#define QUIT_EVENT      (SDL_USEREVENT + 2)     // 退出事件

//定义分辨率
// YUV像素分辨率
#define YUV_WIDTH   1280
#define YUV_HEIGHT  720
//定义YUV格式
#define YUV_FORMAT  SDL_PIXELFORMAT_IYUV

int s_thread_exit = 0;  // 退出标志 = 1则退出
// 用于控制视频的播放速率
int refresh_video_timer(void *data)
{
    while (!s_thread_exit)
    {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(30);
    }
    s_thread_exit = 0;
    //push quit event
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

void YUV_Player(){
    //初始化 SDL
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return ;
    }

    // SDL
    SDL_Event event;                            // 事件
    SDL_Rect rect;                              // 矩形
    SDL_Window *window = NULL;                  // 窗口
    SDL_Renderer *renderer = NULL;              // 渲染
    SDL_Texture *texture = NULL;                // 纹理
    SDL_Thread *timer_thread = NULL;            // 请求刷新线程
    uint32_t pixformat = YUV_FORMAT;            // YUV420P，即是SDL_PIXELFORMAT_IYUV

    // 分辨率
    // 1. YUV的分辨率
    int video_width = YUV_WIDTH;
    int video_height = YUV_HEIGHT;
    // 2.显示窗口的分辨率
    int win_width = YUV_WIDTH;
    int win_height = YUV_WIDTH;

    // YUV文件句柄
    FILE *video_fd = NULL;
    const char *yuv_path = "out.yuv";

    size_t video_buff_len = 0;

    uint8_t *video_buf = NULL; //读取数据后先把放到buffer里面

    // 我们测试的文件是YUV420P格式
    uint32_t y_frame_len = video_width * video_height;
    uint32_t u_frame_len = video_width * video_height / 4;
    uint32_t v_frame_len = video_width * video_height / 4;
    uint32_t yuv_frame_len = y_frame_len + u_frame_len + v_frame_len;

    //创建窗口
    window = SDL_CreateWindow("Simplest YUV Player",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           video_width, video_height,
                           SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!window)
    {
        fprintf(stderr, "SDL: could not create window, err:%s\n",SDL_GetError());
        goto _FAIL;
    }
    // 基于窗口创建渲染器   这个渲染器是在这个窗口内进行渲染的
    renderer = SDL_CreateRenderer(window, -1, 0);
    // 基于渲染器创建纹理
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                video_width,
                                video_height);

    // 分配空间
    video_buf = (uint8_t*)malloc(yuv_frame_len);
    if(!video_buf)
    {
        fprintf(stderr, "Failed to alloce yuv frame space!\n");
        goto _FAIL;
    }

    // 打开YUV文件
    video_fd = fopen(yuv_path, "rb");
    if( !video_fd )
    {
        fprintf(stderr, "Failed to open yuv file\n");
        goto _FAIL;
    }
    // 创建请求刷新线程
    timer_thread = SDL_CreateThread(refresh_video_timer,
                                    NULL,
                                    NULL);

    while (1)
    {
        // 收取SDL系统里面的事件
        SDL_WaitEvent(&event);

        if(event.type == REFRESH_EVENT) // 画面刷新事件
        {
            video_buff_len = fread(video_buf, 1, yuv_frame_len, video_fd);
            if(video_buff_len <= 0)
            {
                fprintf(stderr, "Failed to read data from yuv file!\n");
                goto _FAIL;
            }
            // 设置纹理的数据 video_width = 320， plane
            // 使用读取到的yuv数据进行渲染
            SDL_UpdateTexture(texture, NULL, video_buf, video_width);

            // 显示区域，可以通过修改w和h进行缩放
            rect.x = 0;
            rect.y = 0;
            //float w_ratio = win_width * 1.0 /video_width;
            //float h_ratio = win_height * 1.0 /video_height;
            // 320x240 怎么保持原视频的宽高比例
            rect.w = video_width;
            rect.h = video_height;
//            rect.w = video_width * 0.5;
//            rect.h = video_height * 0.5;

            // 刷新屏幕
            SDL_RenderClear(renderer);
            // 将纹理的数据拷贝给渲染器
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            // 显示
            SDL_RenderPresent(renderer);
        }
        else if(event.type == SDL_WINDOWEVENT)
        {
            //If Resize
            SDL_GetWindowSize(window, &win_width, &win_height);
            printf("SDL_WINDOWEVENT win_width:%d, win_height:%d\n",win_width,
                   win_height );
        }
        else if(event.type == SDL_QUIT) //退出事件
        {
            s_thread_exit = 1;
        }
        else if(event.type == QUIT_EVENT)
        {
            break;
        }
    }

_FAIL:
    s_thread_exit = 1;      // 保证线程能够退出
    // 释放资源
    if(timer_thread)
        SDL_WaitThread(timer_thread, NULL); // 等待线程退出
    if(video_buf)
        free(video_buf);
    if(video_fd)
        fclose(video_fd);
    if(texture)
        SDL_DestroyTexture(texture);
    if(renderer)
        SDL_DestroyRenderer(renderer);
    if(window)
        SDL_DestroyWindow(window);

    SDL_Quit();

    return;
}

#define PCM_BUFFER_SIZE (1024*2*2*2) // 1024 samples * 2 channels * 2 bytes/sample * 2 frames

// 音频PCM数据缓存
static Uint8 *s_audio_buf = NULL;
// 目前读取的位置
static Uint8 *s_audio_pos = NULL;
// 缓存结束位置
static Uint8 *s_audio_end = NULL;

// 音频回调函数，用于填充音频数据到SDL的音频缓冲区
void fill_audio_pcm(void *data, uint8_t *stream, int len) {
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

// 播放PCM音频文件的函数
void PCMPlayer(const char *filename) {
    int ret = -1;
    FILE *audiofd = NULL;
    SDL_AudioSpec spec;
    size_t read_buffer_len = 0;

    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }

    // 打开音频文件
    audiofd = fopen(filename, "rb");
    if (!audiofd) {
        fprintf(stderr, "Failed to open file\n");
        SDL_Quit();
        return;
    }

    // 分配PCM缓存
    s_audio_buf = (uint8_t*)malloc(PCM_BUFFER_SIZE);
    if (!s_audio_buf) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(audiofd);
        SDL_Quit();
        return;
    }

    // 配置SDL音频规格
    spec.freq = 44100;             // 采样率 44100 Hz
    spec.format = AUDIO_F32SYS;    // 32-bit 浮点数格式
    spec.channels = 1;             // 单声道
    spec.silence = 0;              // 无需设置静音值（不适用浮点格式）
    spec.samples = PCM_BUFFER_SIZE / 8; // 每次音频回调处理的样本数量
    spec.callback = fill_audio_pcm; // 音频数据填充回调函数
    spec.userdata = NULL;          // 回调函数数据（未使用）

    // 打开SDL音频设备
    if (SDL_OpenAudio(&spec, NULL)) {
        fprintf(stderr, "Failed to open audio device\n");
        SDL_Quit();
        fclose(audiofd);
        free(s_audio_buf);
        return;
    }

    // 开始播放音频
    SDL_PauseAudio(0);

    // 读取音频数据并播放
    int data_count = 0;
    while (1) {
        // 从文件读取PCM数据到缓存
        read_buffer_len = fread(s_audio_buf, 1, PCM_BUFFER_SIZE, audiofd);
        if (read_buffer_len == 0) {
            break; // 读取完毕，退出循环
        }
        data_count += read_buffer_len;
        printf("Now playing %10d bytes data.\n", data_count);

        // 设置缓存的结束位置和起始位置
        s_audio_end = s_audio_buf + read_buffer_len;
        s_audio_pos = s_audio_buf;

        // 等待直到所有数据被播放
        while (s_audio_pos < s_audio_end) {
            SDL_Delay(20); // 暂停10毫秒，以防CPU过度使用
        }
    }

    // 播放完成
    printf("Finish\n");
    SDL_CloseAudio();   // 关闭音频设备
    free(s_audio_buf); // 释放音频缓存
    fclose(audiofd);   // 关闭文件
    SDL_Quit();        // 退出SDL
    return;
}

// 输入MP4文件，提取视频流并解码生成YUV数据调用SDL进行播放
void VideoPlayer(const char *filename){
    AVFormatContext *fmt_ctx = NULL;
    int videoStream;
    uint32_t i;
    AVCodecContext *CodecCtx = NULL;

    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket pkt;

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
    // 检查是否有视频流信息
    videoStream = -1;
    for(i=0 ; i<fmt_ctx->nb_streams ; i++){
        if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1){
        avformat_close_input(&fmt_ctx);
        return;
    }

    codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
    if(codec == NULL){
        avformat_close_input(&fmt_ctx);
        printf("Can't find decoder\n");
        return;
    }
    // 根据找到的解码器从而分配对应的解码器上下文用于解码中的数据缓存
    CodecCtx = avcodec_alloc_context3(codec);
    // 将流的编解码参数设置到这个上下文中
    avcodec_parameters_to_context(CodecCtx,fmt_ctx->streams[videoStream]->codecpar);
    if(avcodec_open2(CodecCtx,codec,NULL) < 0){
       printf("Failed to open decoder\n");
       avformat_close_input(&fmt_ctx);
       avcodec_close(CodecCtx);
       return;
    }

    frame = av_frame_alloc();
    int ret = 0;
    av_init_packet(&pkt);


    if(SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return ;
    }

    // SDL
    SDL_Event event;                            // 事件
    SDL_Rect rect;                              // 矩形
    SDL_Window *window = NULL;                  // 窗口
    SDL_Renderer *renderer = NULL;              // 渲染
    SDL_Texture *texture = NULL;                // 纹理
    uint32_t pixformat = YUV_FORMAT;            // YUV420P，即是SDL_PIXELFORMAT_IYUV

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

    // 主线程负责解码并将解码后生成的一帧YUV数据放入到消息队列中
    while(av_read_frame(fmt_ctx,&pkt) >= 0){
        if(pkt.stream_index == videoStream){
            if((ret = avcodec_send_packet(CodecCtx,&pkt)) >= 0){
                // 并不会每次send_packet就会立刻能receive_frame，可能需要多次send才能Receive
                while(avcodec_receive_frame(CodecCtx,frame) >= 0){
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
            }
            else{
                fprintf(stderr,"avcodec_receive_frame: %s\n",av_get_err(ret));
            }
        }
        av_packet_unref(&pkt);
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT){
            break;
        }
    }
    printf("Finish w: %d   h: %d\n",CodecCtx->width,CodecCtx->height);
    avformat_close_input(&fmt_ctx);
    avcodec_close(CodecCtx);
    av_packet_unref(&pkt);
    SDL_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return;
}

// SDL音频回调函数
void Fill_audio_pcm(void *userdata, Uint8 *stream, int len) {
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

void AudioPlayer(const char *filename) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *CodecCtx = NULL;
    const AVCodec *Codec = NULL;
    AVPacket pkt;
    AVFrame *frame = NULL;
    int audioStream = -1;
    int ret;
    uint8_t *audio_data = NULL;
    int data_size;

    // 打开媒体文件
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file %s\n", filename);
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Failed to find stream info\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 查找音频流
    for (uint8_t i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            break;
        }
    }
    if (audioStream == -1) {
        fprintf(stderr, "Failed to find Audio Stream\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    Codec = avcodec_find_decoder(fmt_ctx->streams[audioStream]->codecpar->codec_id);
    if (Codec == NULL) {
        fprintf(stderr, "Failed to find decoder\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    CodecCtx = avcodec_alloc_context3(Codec);
    if (CodecCtx == NULL) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    if (avcodec_parameters_to_context(CodecCtx, fmt_ctx->streams[audioStream]->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to context\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    if (avcodec_open2(CodecCtx, Codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        fprintf(stderr, "Failed to allocate frame\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        av_frame_free(&frame);
        avcodec_free_context(&CodecCtx);
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 配置SDL音频规格
    SDL_AudioSpec spec;
    spec.freq = CodecCtx->sample_rate;
    spec.format = AUDIO_S32SYS; // 16-bit signed integer format
    spec.channels = CodecCtx->ch_layout.nb_channels;
    spec.silence = 0;
    spec.samples = PCM_BUFFER_SIZE / 2; // 每次音频回调处理的样本数量
    spec.callback = Fill_audio_pcm;
    spec.userdata = NULL;

    // 打开SDL音频设备
    if (SDL_OpenAudio(&spec, NULL)) {
        fprintf(stderr, "Failed to open audio device\n");
        SDL_Quit();
        av_frame_free(&frame);
        avcodec_free_context(&CodecCtx);
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 开始播放音频
    SDL_PauseAudio(0);


    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        break;
        if (pkt.stream_index == audioStream) {
        }
        av_packet_unref(&pkt);
    }

    // 清理资源
    av_frame_free(&frame);
    avcodec_free_context(&CodecCtx);
    avformat_close_input(&fmt_ctx);
    SDL_CloseAudio();
    SDL_Quit();
}
