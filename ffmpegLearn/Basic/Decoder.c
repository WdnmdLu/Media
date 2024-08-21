#include "Decoder.h"
#include <stdio.h>
void Decoder(){
    printf("Hello World\n");
}

void Dexumer(){
    const char *filename = "believe.flv";

    AVFormatContext *fmt_ctx = NULL;
    int videoIndex = -1;
    int audioIndex = -1;

    int ret = avformat_open_input(&fmt_ctx,filename,NULL,NULL);
    if(ret < 0){
        char buff[1024]={0};
        av_strerror(ret,buff,sizeof(buff)-1);
        printf("Open %s failed: %s\n",filename,buff);
        return;
    }

    ret = avformat_find_stream_info(fmt_ctx,NULL);
    if(ret < 0){
        char buff[1024]={0};
        av_strerror(ret,buff,sizeof(buff)-1);
        printf("Open %s failed: %s\n",filename,buff);
        return;
    }

    printf("av_dump_format %s\n",filename);
    av_dump_format(fmt_ctx,0,filename,0);

    printf("Media name: %s  stream number: %d\n",fmt_ctx->url,fmt_ctx->nb_streams);

    printf("Media average ratio:%lldkbps\n",(int64_t)(fmt_ctx->bit_rate)/1024);
    int total_secnods,hour,minute,second;
    total_secnods = (fmt_ctx->duration) / AV_TIME_BASE;
    hour = total_secnods / 3600;
    minute = (total_secnods % 3600) / 60;
    second = (total_secnods % 60);
    printf("Total duration: %02d:%02d:%02d\n",hour,minute,second);

    for(uint32_t i=0 ; i<fmt_ctx->nb_streams;i++){
        AVStream *in_stream = fmt_ctx->streams[i];
        if(AVMEDIA_TYPE_AUDIO == in_stream->codecpar->codec_type)
        {
            audioIndex = i;
            printf("-----------Audio info------------\n");
            printf("Index:%d\n",in_stream->index);
            printf("Samplerate:%dHz\n",in_stream->codecpar->sample_rate);
            // 判断音频的采样格式
            if (AV_SAMPLE_FMT_FLTP == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_FLTP\n");
            }
            else if (AV_SAMPLE_FMT_S16P == in_stream->codecpar->format)
            {
                printf("sampleformat:AV_SAMPLE_FMT_S16P\n");
            }
            // channels: 音频信道数目
            printf("channel number:%d\n", in_stream->codecpar->ch_layout.nb_channels);
            // codec_id: 音频压缩编码格式
            if (AV_CODEC_ID_AAC == in_stream->codecpar->codec_id)
            {
                printf("audio codec:AAC\n");
            }
            else if (AV_CODEC_ID_MP3 == in_stream->codecpar->codec_id)
            {
                printf("audio codec:MP3\n");
            }
            else
            {
                printf("audio codec_id:%d\n", in_stream->codecpar->codec_id);
            }
        }
        else if(AVMEDIA_TYPE_VIDEO == in_stream->codecpar->codec_type){
            printf("----- Video info: -------\n");
            printf("Index:%d\n",in_stream->index);
            printf("fps:%lffps\n",av_q2d(in_stream->avg_frame_rate));
            if (AV_CODEC_ID_MPEG4 == in_stream->codecpar->codec_id) //视频压缩编码格式
            {
                printf("video codec:MPEG4\n");
            }
            else if (AV_CODEC_ID_H264 == in_stream->codecpar->codec_id) //视频压缩编码格式
            {
                printf("video codec:H264\n");
            }
            else
            {
                printf("video codec_id:%d\n", in_stream->codecpar->codec_id);
            }

            printf("width:%d height:%d\n",in_stream->codecpar->width,
                   in_stream->codecpar->height);

            videoIndex = i;
        }
    }
    printf("AudioIndex:%d   VideoIndex:%d\n",audioIndex,videoIndex);
}


void AAC(){
    int ret = -1;
    char errors[1024];

    char *filename = NULL;
    char *accfile = NULL;

    FILE *aac_fd = NULL;

    int audio_index = -1;
    av_log_set_level(AV_LOG_DEBUG);

}
