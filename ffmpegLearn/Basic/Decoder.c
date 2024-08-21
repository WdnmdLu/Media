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


void extract_h264(const char *filename){
    AVFormatContext *fmt_ctx = NULL;
    AVPacket *pkt = av_packet_alloc();

    FILE *output_file = fopen("out.h264","wb");

    if(!output_file){
        fprintf(stderr,"Could not open outputfile out.h264\n");
        return;
    }

    if(avformat_open_input(&fmt_ctx,filename,NULL,NULL) < 0){
        fprintf(stderr,"Could not open input file: %s\n",filename);
        fclose(output_file);
        return;
    }

    if(avformat_find_stream_info(fmt_ctx,NULL) < 0){
        fprintf(stderr,"Could not find stream info\n");
        avformat_close_input(&fmt_ctx);
        fclose(output_file);
        return;
    }

    int videoIndex = -1;
    uint8_t i = 0;
    for(i = 0;i<fmt_ctx->nb_streams ; i++){
        if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoIndex = -1;
            break;
        }
    }

    if(videoIndex == -1){
        fprintf(stderr,"Could not find video stream\n");
        avformat_close_input(&fmt_ctx);
        fclose(output_file);
        return;
    }

    AVPacket spsPacket, ppsPacket, tmpPacket;
    uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t sendSpsPps = 0;

    while (av_read_frame(fmt_ctx, pkt) == 0) {
        // 根据pkt->stream_index判断是不是视频流
        if (pkt->stream_index == videoIndex) {
            // 仅1次处理sps pps
            if (!sendSpsPps) {
                int spsLength = 0;
                int ppsLength = 0;
                uint8_t *ex = fmt_ctx->streams[videoIndex]->codecpar->extradata;


                // 获取SPS和PPS长度
                spsLength = (ex[6] << 8) | ex[7];
                ppsLength = (ex[8 + spsLength + 1] << 8) | ex[8 + spsLength + 2];


                // 为spsPacket和ppsPacket的数据分配内存
                av_new_packet(&spsPacket, spsLength + 4);
                av_new_packet(&ppsPacket, ppsLength + 4);


                // 拼接SPS
                memcpy(spsPacket.data, startCode, 4);
                memcpy(spsPacket.data + 4, ex + 8, spsLength);
                fwrite(spsPacket.data, 1, spsPacket.size, output_file); // 写入SPS到文件


                // 拼接PPS
                memcpy(ppsPacket.data, startCode, 4);
                memcpy(ppsPacket.data + 4, ex + 8 + spsLength + 2 + 1, ppsLength);
                fwrite(ppsPacket.data, 1, ppsPacket.size, output_file); // 写入PPS到文件


                sendSpsPps = 1;
            }


            // 处理读到pkt中的数据
            int nalLength = 0;
            uint8_t *data = pkt->data;
            while (data < pkt->data + pkt->size) {
                nalLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                if (nalLength > 0) {
                    memcpy(data, startCode, 4);  // 添加起始码
                    tmpPacket = *pkt;      // 复制packet
                    tmpPacket.data = data;         // 设置指针偏移
                    tmpPacket.size = nalLength + 4; // 更新大小


                    fwrite(tmpPacket.data, 1, tmpPacket.size, output_file); // 写入NALU到文件
                }
                data += 4 + nalLength; // 移动到下一个NALU
            }
        }


        av_packet_unref(pkt);
    }

    fclose(output_file);
    av_packet_free(&pkt);
    avformat_close_input(&fmt_ctx);
    printf("Finish\n");
}
