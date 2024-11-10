#include "player.h"
#include <iostream>
#include <functional>
#include <math.h>
#include <openglwidget.h>

#if !defined(MIN)
    #define MIN(A,B)	((A) < (B) ? (A) : (B))
#endif

#define Range 200
// 音频PCM数据缓存
static Uint8 *s_audio_buf = NULL;
// 目前读取的位置
static Uint8 *s_audio_pos = NULL;
// 缓存结束位置
static Uint8 *s_audio_end = NULL;
// 记录当前播放的样本数
static uint8_t sampleCount = 0;
// 每个音频样本的播放时间
static double PerSampleTime = 0;

void fill_audio_pcm(void *data, uint8_t *stream, int len){
    Player* p = (Player*)data;
    //应该在这里更新timeStamp
    // 清空音频流
    SDL_memset(stream, 0, len);
    // 检查是否已经没有更多数据
    if (s_audio_pos >= s_audio_end) {
        sampleCount = 0;
        return;
    }
    p->AudioTimeStamp += PerSampleTime;
    if(p->AudioTimeStamp - p->LastAudioTimeStamp >= 1){
        //更新进度条
        p->updateCallback();
        p->LastAudioTimeStamp = p->AudioTimeStamp;
    }

    // 计算实际可用的数据长度
    int remain_buffer_len = s_audio_end - s_audio_pos;
    len = (len < remain_buffer_len) ? len : remain_buffer_len;
    // 将数据从缓存填充到音频流
    SDL_MixAudio(stream, s_audio_pos, len, SDL_MIX_MAXVOLUME / 8);
    // 更新缓存中的当前位置
    s_audio_pos += len;
}

Player::Player()
{
    //SDL初始化
    SDL_Init(SDL_INIT_AUDIO);

    pthread_mutex_init(&Mut,NULL);
    pthread_cond_init(&Cond,NULL);

    pthread_mutex_init(&timeMut,NULL);
    pthread_cond_init(&timeCond,NULL);
}

Player::~Player(){
    printf("~Player\n");
    fflush(NULL);
}
// 设置播放的媒体文件路径
void Player::SetFilePath(QString path){
    printf("Begin to set filePath\n");
    fflush(NULL);
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
    //设置好了媒体文件后就根据解析的上下文初始化音频
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
        if(this->state == FINISH && this->VideoQueue.GetSize() == 0){
            printf("VideoFinishPlay\n");
            fflush(NULL);
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
        }
        if(this->state == SEEK || this->state == STOP){
            printf("VideoWait\n");
            fflush(NULL);
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
            printf("Video WakeUp\n");
            fflush(NULL);
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
            //传入的是转化后的帧
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
        if(this->state == FINISH && this->state == FINISH){
            printf("AudioFinishPlay\n");
            fflush(NULL);
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
        }

        if(this->state == SEEK || this->state == STOP){
            printf("AudioWait\n");
            fflush(NULL);
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
            printf("Audio WakeUp\n");
            fflush(NULL);
        }
        else if(this->state == EXIT){

        }
        else if(this->AudioQueue.GetSize() == 0){
            SDL_Delay(100);
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
            printf("ReadWait\n");
            fflush(NULL);
            //等待被唤醒
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
            printf("ReadWakeUp\n");
            fflush(NULL);
        }
        if(AudioQueue.GetSize() >= 200 || VideoQueue.GetSize() >= 200){
            //队列满了，等待消耗然后在接着运行
            SDL_Delay(100);
        }

        AVPacket* pkt = (AVPacket*)malloc(sizeof(AVPacket));
        static int ret;
        ret = av_read_frame(fmt_ctx,pkt);
        if(ret < 0){
            printf("ReadThreadFinishRead\n");
            fflush(NULL);
            //数据已经读取完成了，退出线程
            this->state = FINISH;
            pthread_cond_wait(&Cond,&Mut);
            pthread_mutex_unlock(&Mut);
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
    int diff = (CurrentVideoTimeStamp - AudioTimeStamp)*1000;
    int delay = (CurrentVideoTimeStamp - LastVideoTimeStamp)*1000;
    if(doSeek){
        printf("Wait\n");
        fflush(NULL);
        pthread_cond_wait(&timeCond,&timeMut);
        pthread_mutex_unlock(&timeMut);
        diff = (CurrentVideoTimeStamp - AudioTimeStamp)*1000;
        delay = diff;
        doSeek = false;
    }
    if(std::abs(diff) < 5){
        SDL_Delay(delay);
    }
    else if(diff < -5){
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
    if(doSeek){
        pthread_cond_signal(&timeCond);
        printf("Signal\n");
        fflush(NULL);
    }
}

void copyDecodedFrame420(uint8_t* src, uint8_t* dist,int linesize, int width, int height)
{
    width = MIN(linesize, width);
    for (int i = 0; i < height; ++i) {
        memcpy(dist, src, width);
        dist += width;
        src += linesize;
    }
}
// 传入的帧就是已经经过转化后的帧了
void Player::ShowPicture(AVFrame* frame){
    //在这里调用OpenGL的接口进行渲染
    unsigned int lumaLength= vCodecCtx->height * (MIN(frame->linesize[0], vCodecCtx->width));
    unsigned int chromBLength=((vCodecCtx->height)/2)*(MIN(frame->linesize[1], (vCodecCtx->width)/2));
    unsigned int chromRLength=((vCodecCtx->height)/2)*(MIN(frame->linesize[2], (vCodecCtx->width)/2));

    H264YUV_Frame *updateYUVFrame = new H264YUV_Frame();

    updateYUVFrame->luma.length = lumaLength;
    updateYUVFrame->chromaB.length = chromBLength;
    updateYUVFrame->chromaR.length =chromRLength;

    updateYUVFrame->luma.dataBuffer=(unsigned char*)malloc(lumaLength);
    updateYUVFrame->chromaB.dataBuffer=(unsigned char*)malloc(chromBLength);
    updateYUVFrame->chromaR.dataBuffer=(unsigned char*)malloc(chromRLength);

    copyDecodedFrame420(frame->data[0],updateYUVFrame->luma.dataBuffer,frame->linesize[0],
              vCodecCtx->width,vCodecCtx->height);
    copyDecodedFrame420(frame->data[1], updateYUVFrame->chromaB.dataBuffer,frame->linesize[1],
              vCodecCtx->width / 2,vCodecCtx->height / 2);
    copyDecodedFrame420(frame->data[2], updateYUVFrame->chromaR.dataBuffer,frame->linesize[2],
              vCodecCtx->width / 2,vCodecCtx->height / 2);
    updateYUVFrame->width=vCodecCtx->width;
    updateYUVFrame->height=vCodecCtx->height;

    // 开始进行渲染
    this->RenderCallback(updateYUVFrame);

    // 渲染完成之后释放这一帧数据
    if(updateYUVFrame->luma.dataBuffer){
        free(updateYUVFrame->luma.dataBuffer);
        updateYUVFrame->luma.dataBuffer=NULL;
    }

    if(updateYUVFrame->chromaB.dataBuffer){
        free(updateYUVFrame->chromaB.dataBuffer);
        updateYUVFrame->chromaB.dataBuffer=NULL;
    }

    if(updateYUVFrame->chromaR.dataBuffer){
        free(updateYUVFrame->chromaR.dataBuffer);
        updateYUVFrame->chromaR.dataBuffer=NULL;
    }

    if(updateYUVFrame){
        delete updateYUVFrame;
        updateYUVFrame = NULL;
    }
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
    sampleCount = 0;
}

void Player::SDLAudioInit(){
    spec.freq = aCodecCtx->sample_rate;             // 采样率 44100 Hz
    spec.format = AUDIO_F32SYS;    // 32-bit 浮点数格式
    spec.channels = 2;             // 单声道
    spec.silence = 0;              // 无需设置静音值（不适用浮点格式）
    spec.samples = 1024; // 每次音频回调处理的样本数量
    spec.callback = fill_audio_pcm; // 音频数据填充回调函数
    spec.userdata = (void*)this;          // 回调函数数据（未使用）
    SDL_OpenAudio(&spec, NULL);
    s_audio_buf = (uint8_t*)malloc(aCodecCtx->sample_rate*2*4);
    PerSampleTime = (double)1024/(double)aCodecCtx->sample_rate;
    //进行四舍五入，只保留小数点后3位
    PerSampleTime = round(PerSampleTime * 1000) / 1000;
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
    pthread_cond_broadcast(&Cond);
    printf("DoWakeUp\n");
    fflush(NULL);
}

void Player::Exit(){
    this->state = EXIT;
}

void Player::Seek(int Pos){
    this->state = SEEK;
    // 关闭音频播放
    SDL_PauseAudio(1);
    // 清空音频当前播放的数据
    s_audio_pos = s_audio_end;
    AudioTimeStamp = 0.0f;
    CurrentVideoTimeStamp = 0.0f;
    LastVideoTimeStamp = 0.0f;
    LastAudioTimeStamp = 0.0f;
    int64_t VideoTimeStamp = Pos/av_q2d(fmt_ctx->streams[videoIndex]->time_base);
    int64_t AudioTimeStamp = Pos/av_q2d(fmt_ctx->streams[audioIndex]->time_base);

    //清空解码器上下文: aCodecCtx vCodecCtx
    avcodec_flush_buffers(aCodecCtx);
    avcodec_flush_buffers(vCodecCtx);

    av_seek_frame(fmt_ctx,audioIndex,AudioTimeStamp,AVSEEK_FLAG_ANY);
    av_seek_frame(fmt_ctx,videoIndex,VideoTimeStamp,AVSEEK_FLAG_BACKWARD);

    this->AudioQueue.ClearQueue();
    this->VideoQueue.ClearQueue();

    doSeek = true;
    SDL_PauseAudio(0);
    this->state = PLAYER;
    pthread_cond_broadcast(&Cond);
    printf("Finish seek\n");
    fflush(NULL);
}
