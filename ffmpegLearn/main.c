#include <stdio.h>
#include <libavformat/avformat.h>

int main(int argc,char *argv[]){
    const char *default_filename = "believe.mp4";
    char *in_filename = NULL;
    if(argc<2){
        in_filename = default_filename;
    }
    else{
        in_filename = argv[1];
    }
    printf("in_filename = %s\n",in_filename);

    // AVFormatContext是描述一个媒体文件或媒体流的构成和基本信息的结构体
    AVFormatContext *ifmt_ctx = NULL;

    int videoIndex = -1; //视频索引
    int audioIndex = -1; //音频索引

    int ret = avformat_open_input(&ifmt_ctx, in_filename,NULL,NULL);
    if(ret < 0){
        char buff[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("Open %s failed:%s\n",in_filename,buff);
        goto failed;
    }
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if(ret <0 ){
        char buff[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("avformat_find_stream_info %s failed:%s\n",in_filename,buff);
        goto failed;
    }

    //打开媒体文件成功
    printf_s("\n==== av_dump_format in_filename:%s ===\n", in_filename);
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf_s("\n==== av_dump_format finish =======\n\n");
    // url: 调用avformat_open_input读取到的媒体文件的路径/名字
    printf("media name:%s\n", ifmt_ctx->url);
    // nb_streams: nb_streams媒体流数量
    printf("stream number:%d\n", ifmt_ctx->nb_streams);
    // bit_rate: 媒体文件的码率,单位为bps
    printf("media average ratio:%lldkbps\n",(int64_t)(ifmt_ctx->bit_rate/1024));
    // 时间
    int total_seconds, hour, minute, second;
    // duration: 媒体文件时长，单位微妙
    total_seconds = (ifmt_ctx->duration) / AV_TIME_BASE;  // 1000us = 1ms, 1000ms = 1秒
    hour = total_seconds / 3600;
    minute = (total_seconds % 3600) / 60;
    second = (total_seconds % 60);
    //通过上述运算，可以得到媒体文件的总时长
    printf("total duration: %02d:%02d:%02d\n", hour, minute, second);
    printf("\n");
    //分别把流都读取出来
    for(uint32_t i = 0;i<ifmt_ctx->nb_streams;i++){
        AVStream *in_stream = ifmt_ctx->streams[i];
        //根据codec_type去判断是属于哪个流 音频，视频，字幕等等流
        if(AVMEDIA_TYPE_AUDIO == in_stream->codecpar->codec_type){
            printf("----- Audio info:\n");
            //index: 每个流成分在ffmpeg解复用分析后都有唯一的index作为标识
            printf("index:%d\n",in_stream->index);
            // sample_rate: 音频编解码器的采样率，单位Hz
            // codecpar->format：音频采样格式
            if(AV_SAMPLE_FMT_FLTP == in_stream->codecpar->format){
                printf("SampleFormat:")
            }
        }
    }

failed:
    if(ifmt_ctx)
        avformat_close_input(&ifmt_ctx);


    getchar(); //加上这一句，防止程序打印完信息马上退出
    return 0;
}