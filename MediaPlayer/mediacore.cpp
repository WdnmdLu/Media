#include "mediacore.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>

std::string formatDuration(int64_t duration) {
    int hours, mins, secs;
    secs = duration / 1000000; // 总秒数
    mins = secs / 60;         // 总分钟数
    secs %= 60;               // 剩余秒数
    hours = mins / 60;        // 总小时数
    mins %= 60;               // 剩余分钟数

    // 格式化为 xx:xx 或 xx:xx:xx
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", (int)mins, (int)secs);
    return std::string(buffer);
}

MediaCore::MediaCore(int w,int h,void *win)
{
    videoIndex = -1;
    audioIndex = -1;
    filePath = "";
    videoQueue = new Queue();
    audioQueue = new Queue();
    this->winWidth = w;
    this->winHeight = h;
    videoSwsCtx = NULL;
    this->sts = NONE;

    videoFrame = av_frame_alloc();
    audioFrame = av_frame_alloc();

    sdl = new SdlShow(w,h,win);
    pthread_mutex_init(&Mut1,NULL);
    pthread_cond_init(&Cond1,NULL);

    pthread_mutex_init(&Mut2,NULL);
    pthread_cond_init(&Cond2,NULL);

    pthread_mutex_init(&Mut3,NULL);
    pthread_cond_init(&Cond3,NULL);
}

MediaCore::~MediaCore(){

}

int MediaCore::Init(){
    if(filePath.empty()){
        return -1;
    }
    fmt_ctx = avformat_alloc_context();
    int ret = avformat_open_input(&fmt_ctx, filePath.c_str(), NULL, NULL);
    if (ret < 0) {
        char err_buf[128];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cout << "Could not open source file " << filePath << ": " << err_buf << std::endl;
        return ret;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
       char err_buf[128];
       av_strerror(ret, err_buf, sizeof(err_buf));
       std::cout << "Failed to retrieve input stream information: " << err_buf << std::endl;
       avformat_close_input(&fmt_ctx);
       return ret;
    }

    av_dump_format(fmt_ctx,0,filePath.c_str(),0);

    uint8_t i;
    for( i = 0 ; i < fmt_ctx->nb_streams ; i++){
        if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoIndex = i;
            const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            videoCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(videoCodecCtx,fmt_ctx->streams[i]->codecpar);
            avcodec_open2(videoCodecCtx,codec,NULL);
        }
        else if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audioIndex = i;
            const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            audioCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(audioCodecCtx,fmt_ctx->streams[i]->codecpar);
            avcodec_open2(audioCodecCtx,codec,NULL);
        }
    }


    if(videoIndex == -1 || audioIndex == -1){
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    sws_getContext(fmt_ctx->streams[videoIndex]->codecpar->width,// 原视频宽度
                   fmt_ctx->streams[videoIndex]->codecpar->height,// 原视频高度
                   (AVPixelFormat)fmt_ctx->streams[videoIndex]->codecpar->format,// 原视频的格式
                   this->winWidth,this->winHeight,// 转化后图片的宽高
                   AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL,NULL,NULL);

    videoFrame->width = winWidth;
    videoFrame->height = winHeight;
    videoFrame->format = AV_PIX_FMT_YUV420P;

    std::cout<<"Init success"<<std::endl;

    int64_t duration = fmt_ctx->duration;
    total_seconds = (int)duration / 1e6; // 转换为秒
    durationStr = formatDuration(duration);
    return 0;
}

void MediaCore::SetPath(const char *Path){
    filePath = Path;
    std::cout<<filePath<<std::endl;
}

void MediaCore::unInit(){
    if(fmt_ctx != NULL){
        avformat_close_input(&fmt_ctx);
        avcodec_close(videoCodecCtx);
        avcodec_close(audioCodecCtx);
        SDL_Quit();
    }
    delete sdl;
}

// 解复用线程，用于解析输入的媒体文件，读取音频流和视频流，分别放入对应的包队列中
void* DexumerThread(void* p){
    MediaCore* core = (MediaCore*)p;
    int ret = -1;
    static int count = 0;
    while(core->sts != EXIT){
        pthread_mutex_lock(&core->Mut1);
        while(core->sts == STOP){
            pthread_cond_wait(&core->Cond1,&core->Mut1);
        }
        pthread_mutex_unlock(&core->Mut1);
        if(core->videoQueue->GetSize() > 50 || core->audioQueue->GetSize() > 50){
            fflush(NULL);
            SDL_Delay(10);
            count++;
            if(count == 100){
                printf("VideoQueue: %d  AudioQueue: %d\n",core->videoQueue->GetSize(),core->audioQueue->GetSize());
                count = 0;
            }
            continue;
        }
        AVPacket *pkt = (AVPacket*)malloc(sizeof(AVPacket));
        ret = av_read_frame(core->fmt_ctx,pkt);
        if(ret < 0){
            free(pkt);
            break;
        }
        if(pkt->stream_index == core->videoIndex){
            core->videoQueue->Push(pkt);
        }
        else if(pkt->stream_index == core->audioIndex){
            core->audioQueue->Push(pkt);
        }
    }
    return NULL;
}
// 视频解码线程
void* VideoThread(void *p){
    MediaCore* core = (MediaCore*)p;
    AVFrame *frame = av_frame_alloc();
    while(core->sts != EXIT){
        pthread_mutex_lock(&core->Mut2);
        while(core->sts == STOP){
            pthread_cond_wait(&core->Cond2,&core->Mut2);
        }
        pthread_mutex_unlock(&core->Mut2);

        if(core->videoQueue->GetSize() == 0){
            usleep(100);
            continue;
        }
        int ret;
        avcodec_send_packet(core->videoCodecCtx,(AVPacket*)core->videoQueue->Front());
        core->videoQueue->Pop();
        while((ret = avcodec_receive_frame(core->videoCodecCtx,frame)) >= 0){
            core->VideoDelay(frame->pts);
            //做视频渲染
            core->sdl->ShowPicture(frame);
        }

    }
    return NULL;
}
// 音频解码线程
void* AudioThread(void *p){
    SDL_PauseAudio(0);
    MediaCore* core = (MediaCore*)p;
    AVFrame *frame = av_frame_alloc();

    while(core->sts != EXIT){
        pthread_mutex_lock(&core->Mut3);
        while(core->sts == STOP){
            pthread_cond_wait(&core->Cond3,&core->Mut3);
        }
        pthread_mutex_unlock(&core->Mut3);

        if(core->audioQueue->GetSize() == 0){
            usleep(100);
            continue;
        }
        int ret;
        avcodec_send_packet(core->audioCodecCtx,(AVPacket*)core->audioQueue->Front());
        core->audioQueue->Pop();
        while((ret = avcodec_receive_frame(core->audioCodecCtx,frame)) >= 0){
            core->AudioDelay(frame->pts);
            core->sdl->PlayerAudioFrame(frame);
        }

    }
    return NULL;
}

void MediaCore::StartPlayer(){
    // 刚开始播放
    if(this->sts == NONE){
        //创建三个线程
        pthread_t t1,t2,t3;
        pthread_create(&t1,NULL,DexumerThread,this);

        pthread_create(&t2,NULL,VideoThread,this);

        pthread_create(&t3,NULL,AudioThread,this);

        pthread_detach(t1);
        pthread_detach(t2);
        pthread_detach(t3);

        this->sts = PLAYER;
        std::cout<<"Start Ok"<<std::endl;
        return;
    }// 已经有视频正在播放了
    else{

    }

}

void MediaCore::StopPlayer(){
    //暂停视频播放
}

void MediaCore::ExitPlayer(){

}

// 获取视频时间戳
float MediaCore::GetVideoPTS(uint64_t pts){
    return pts*av_q2d(fmt_ctx->streams[videoIndex]->time_base);
}
// 获取音频时间戳
float MediaCore::GetAudioPTS(uint64_t pts){
    return pts*av_q2d(fmt_ctx->streams[audioIndex]->time_base);
}

void MediaCore::VideoDelay(int64_t pts){
    // 获取到当前视频的时间戳
    videoTimeStamp = GetVideoPTS(pts);
    float diffTime = (videoTimeStamp - audioTimeStamp)*1000;
    int sleepTime = (int)diffTime;

    if(sleepTime > - 200 && sleepTime < 200){
        if(sleepTime < 0){
            sleepTime*=-1;
        }
        SDL_Delay(sleepTime);
        return;
    }//视频已经远远落后于音频，不做delay，快速渲染以追上音频
    else if(sleepTime < -200){
        return;
    }
    //视频的时间戳远远的大于了音频，因此需要阻塞等待直到视频于音频时间戳同步
    while(sleepTime > 200){
        SDL_Delay(200);
        //延时之后在进行判断
        diffTime = (videoTimeStamp - audioTimeStamp)*1000;
        sleepTime = (int)diffTime;
        //延时200ms之后在做判断
        // 处于正常的时间，正常延时然后返回
        if(sleepTime > - 200 && sleepTime < 200){
            if(sleepTime < 0){
                sleepTime*=-1;
            }
            SDL_Delay(sleepTime);
            return;
        }//视频已经远远落后于音频，不做delay，快速渲染以追上音频
        else if(sleepTime < -200){
            return;
        }
        //否则接着进行延时，直到音频追上了视频
    }
    return;
}

void MediaCore::AudioDelay(int64_t pts){
    audioTimeStamp = GetAudioPTS(pts);
    int diffTime = (int)(audioTimeStamp - audioLastTimeStamp);
    static int time = 0;
    if(diffTime >= 1){
        time++;
        // 更新播放的进度条
        this->updateCallback();
        printf("Time: %d\n",time);
        audioLastTimeStamp = audioTimeStamp;
        fflush(NULL);
    }

    return;
}
// 传入的Time是s，要进行转化
void MediaCore::Seek(int Time) {
    //暂停视频的播放和解复用
    this->sts = STOP;
    pthread_mutex_lock(&Mut1);
    pthread_mutex_lock(&Mut2);
    pthread_mutex_lock(&Mut3);

    int ret = av_seek_frame(fmt_ctx, -1, Time * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
    if(ret < 0){
        printf("Error\n");
        return;
    }
    //清空音频队列和视频队列
    videoQueue->ClearQueue();
    audioQueue->ClearQueue();
    printf("Seek Ok\n");
    fflush(NULL);
    sts = PLAYER;
    pthread_mutex_unlock(&Mut1);
    pthread_mutex_unlock(&Mut2);
    pthread_mutex_unlock(&Mut3);
    //唤醒音频和视频线程
    pthread_cond_signal(&Cond1);
    pthread_cond_signal(&Cond2);
    pthread_cond_signal(&Cond3);
}
