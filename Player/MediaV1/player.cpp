#include "player.h"
#include <iostream>
#include <functional>
#define Range 200
// 音频PCM数据缓存
static Uint8 *s_audio_buf = NULL;
// 目前读取的位置
static Uint8 *s_audio_pos = NULL;
// 缓存结束位置
static Uint8 *s_audio_end = NULL;

void fill_audio_pcm(void *data, uint8_t *stream, int len){
    //应该在这里更新timeStamp

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

Player::Player(void* win)
{
    //SDL初始化
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);

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
                                width,
                                height);
    s_audio_buf = (uint8_t*)malloc(1024*2*2*2);
}

Player::~Player(){
    printf("~Player\n");
    fflush(NULL);
}

void Player::SetFilePath(QString path){
    if(this->filePath.isEmpty()){
        this->filePath = path;
        InitDecode();
    }
    else{
        this->filePath = path;
        SDL_CloseAudio();
        DeInitDecode();
        InitDecode();
    }
    SDLAudioInit();

    std::cout<<"Init success"<<std::endl;

    int64_t duration = fmt_ctx->duration; // 获取总时长，单位是微秒

    // 转换为秒
    TotalTime = (int)(duration * 1.0 / AV_TIME_BASE);

}

int Player::VideoThread(){
    printf("VideoThread\n");
    fflush(NULL);
    AVFrame* frame = av_frame_alloc();
    AVFrame* dFrame = av_frame_alloc();
    dFrame->width = width;
    dFrame->height = height;
    dFrame->format = AV_PIX_FMT_YUV420P;

    if (av_frame_get_buffer(dFrame, 32) < 0) {
        fprintf(stderr, "Could not allocate the frame data for dFrame\n");
        av_frame_free(&frame);
        av_frame_free(&dFrame);
        return -1;
    }
    for(;;){
        if(this->state == SEEK || this->state == STOP){
            SDL_Delay(20);
            continue;
        }
        else if(this->state == EXIT){

        }
        else if(this->VideoQueue.GetSize() == 0){
            SDL_Delay(10);
            continue;
        }

        avcodec_send_packet(this->vCodecCtx,(AVPacket*)this->VideoQueue.Front());
        this->VideoQueue.Pop();

        while(avcodec_receive_frame(this->vCodecCtx,frame) >= 0){
            sws_scale(swsCtx,
                      (const uint8_t * const*)frame->data,
                       frame->linesize,
                       0,
                       this->vCodecCtx->height,
                       dFrame->data,
                       dFrame->linesize);
            ShowPicture(dFrame);
            videoDelay(frame);
        }
    }

    return 0;
}

int Player::AudioThread(){
    printf("AudioThread\n");
    fflush(NULL);
    AVFrame* frame = av_frame_alloc();
    SDL_PauseAudio(0);
    for(;;){
        if(this->state == SEEK || this->state == STOP){
            SDL_Delay(20);
            continue;
        }
        else if(this->state == EXIT){

        }
        else if(this->AudioQueue.GetSize() == 0){
            SDL_Delay(10);
            continue;
        }

        avcodec_send_packet(this->aCodecCtx,(AVPacket*)this->AudioQueue.Front());
        this->AudioQueue.Pop();

        while(avcodec_receive_frame(this->aCodecCtx,frame) >= 0){
            audioDelay(frame);
            //接收到一帧解码后的数据
            PlayAudio(frame);
        }
    }

    return 0;
}

int Player::ReadThread(){
    printf("ReadThread\n");
    fflush(NULL);
    for(;;){
        if(this->state == EXIT){
            break;
        }
        else if(this->state == STOP || this->state == SEEK){
            //等待被唤醒
            SDL_Delay(20);
            continue;
        }
        if(AudioQueue.GetSize() >= 100 || VideoQueue.GetSize() >= 100){
            //队列满了，等待消耗然后在接着运行
            SDL_Delay(10);
            continue;
        }

        AVPacket* pkt = (AVPacket*)malloc(sizeof(AVPacket));
        static int ret;
        ret = av_read_frame(fmt_ctx,pkt);
        if(ret < 0){
            //数据已经读取完成了，退出线程
            this->state = FINISH;
            break;
        }
        if(pkt->stream_index == audioIndex){
            this->AudioQueue.Push(pkt);
        }
        else if(pkt->stream_index == videoIndex){
            this->VideoQueue.Push(pkt);
        }
    }
    return 0;
}

bool Player::Begin(){
    if(this->filePath.isEmpty()){
        return false;
    }
    // 创建三个线程开始
    Read = new std::thread(&Player::ReadThread,this);

    Video = new std::thread(&Player::VideoThread,this);

    Audio = new std::thread(&Player::AudioThread,this);
    this->state = PLAYER;
    return true;
}

void Player::videoDelay(AVFrame* frame){
    CurrentVideoTimeStamp = frame->pts*av_q2d(fmt_ctx->streams[videoIndex]->time_base);
    // 计算的是当前的这一帧与音频时间的差值，转化为毫秒
    int diff = (CurrentVideoTimeStamp - AudioTimeStamp)*1000;

    int delay = (CurrentVideoTimeStamp - LastVideoTimeStamp)*1000;
    if(doSeek){
        delay = diff;
        doSeek = false;
    }
    if(std::abs(diff) < 200){
        SDL_Delay(delay);
    }
    else if(diff < -200){
        LastVideoTimeStamp = CurrentVideoTimeStamp;
        return;
    }
    else{
        SDL_Delay(delay*2);
        LastVideoTimeStamp = CurrentVideoTimeStamp;
    }

   /* CurrentVideoTimeStamp = frame->pts*av_q2d(fmt_ctx->streams[videoIndex]->time_base);
    double diff = CurrentVideoTimeStamp - AudioTimeStamp;
    if(diff < 0){
        diff = diff*-1;
    }
    SDL_Delay(diff*1000);*/
}

void Player::audioDelay(AVFrame* frame){
    //更新音频时间戳
    AudioTimeStamp = frame->pts*av_q2d(fmt_ctx->streams[audioIndex]->time_base);

    if(AudioTimeStamp - LastAudioTimeStamp >= 1){
        //更新进度条
        this->updateCallback();
        LastAudioTimeStamp = AudioTimeStamp;
    }
}

void Player::ShowPicture(AVFrame* frame){
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

void Player::PlayAudio(AVFrame* frame){
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

void Player::SDLAudioInit(){
    spec.freq = fmt_ctx->streams[audioIndex]->codecpar->sample_rate;             // 采样率 44100 Hz
    spec.format = AUDIO_F32SYS;    // 32-bit 浮点数格式
    spec.channels = 2;             // 单声道
    spec.silence = 0;              // 无需设置静音值（不适用浮点格式）
    spec.samples = 1024; // 每次音频回调处理的样本数量
    spec.callback = fill_audio_pcm; // 音频数据填充回调函数
    spec.userdata = NULL;          // 回调函数数据（未使用）
    SDL_OpenAudio(&spec, NULL);
}

void Player::InitDecode(){
    //在这里进行初始化ffmpeg
    fmt_ctx = avformat_alloc_context();
    int ret = avformat_open_input(&fmt_ctx, filePath.toStdString().c_str(), NULL, NULL);
    if (ret < 0) {
        char err_buf[128];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cout << "Could not open source file " << filePath.toStdString() << ": " << err_buf << std::endl;
        return;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        char err_buf[128];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cout << "Failed to retrieve input stream information: " << err_buf << std::endl;
        avformat_close_input(&fmt_ctx);
        return;
    }

    av_dump_format(fmt_ctx,0,filePath.toStdString().c_str(),0);

    uint8_t i;
    for( i = 0 ; i < fmt_ctx->nb_streams ; i++){
        if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoIndex = i;
            const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            vCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(vCodecCtx,fmt_ctx->streams[i]->codecpar);
            avcodec_open2(vCodecCtx,codec,NULL);
        }
        else if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audioIndex = i;
            const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            aCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(aCodecCtx,fmt_ctx->streams[i]->codecpar);
            avcodec_open2(aCodecCtx,codec,NULL);
        }
    }


    if(videoIndex == -1 || audioIndex == -1){
        avformat_close_input(&fmt_ctx);
        printf("Error\n");
        return;
    }

    swsCtx = sws_getContext(fmt_ctx->streams[videoIndex]->codecpar->width,// 原视频宽度
                   fmt_ctx->streams[videoIndex]->codecpar->height,// 原视频高度
                   (AVPixelFormat)fmt_ctx->streams[videoIndex]->codecpar->format,// 原视频的格式
                   width,height,// 转化后图片的宽高
                   AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL,NULL,NULL);
}

void Player::DeInitDecode(){
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&aCodecCtx);
    avcodec_free_context(&vCodecCtx);
}

void Player::Stop(){
    this->state = STOP;
    SDL_PauseAudio(1);
}

void Player::Play(){
    this->state = PLAYER;
    SDL_PauseAudio(0);
}

void Player::Exit(){
    this->state = EXIT;
}

void Player::Seek(int Pos){
    this->state = SEEK;
    // 关闭音频播放
    int64_t VideoTimeStamp = Pos/av_q2d(fmt_ctx->streams[videoIndex]->time_base);
    int64_t AudioTimeStamp = Pos/av_q2d(fmt_ctx->streams[audioIndex]->time_base);

    //清空解码器上下文: aCodecCtx vCodecCtx
    avcodec_flush_buffers(aCodecCtx);
    avcodec_flush_buffers(vCodecCtx);

    avformat_seek_file(fmt_ctx, videoIndex, 0, VideoTimeStamp*AV_TIME_BASE, INT64_MAX, 0);
    avformat_seek_file(fmt_ctx, audioIndex, 0, AudioTimeStamp*AV_TIME_BASE, INT64_MAX, 0);

    this->AudioQueue.ClearQueue();
    this->VideoQueue.ClearQueue();

    doSeek = true;

    this->state = PLAYER;
}
