#include "Decoder.h"
// 解码输入的MP4文件，提取其中的音频流数据生成对应的aac文件
#define ADTS_HEADER_LEN  7;
const int sampling_frequencies[] = {
    96000,  // 0x0
    88200,  // 0x1
    64000,  // 0x2
    48000,  // 0x3
    44100,  // 0x4
    32000,  // 0x5
    24000,  // 0x6
    22050,  // 0x7
    16000,  // 0x8
    12000,  // 0x9
    11025,  // 0xa
    8000   // 0xb
    // 0xc d e f是保留的
};

// 解码输入的MP4文件，提取其中的音频流数据生成对应的aac文件
int adts_header(char * const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels)
{

    int sampling_frequency_index = 3; // 默认使用48000hz
    int adtsLen = data_length + 7;

    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for(i = 0; i < frequencies_size; i++)
    {
        if(sampling_frequencies[i] == samplerate)
        {
            sampling_frequency_index = i;
            break;
        }
    }
    if(i >= frequencies_size)
    {
        printf("unsupport samplerate:%d\n", samplerate);
        return -1;
    }

    p_adts_header[0] = 0xff;         //syncword:0xfff                          高8bits
    p_adts_header[1] = 0xf0;         //syncword:0xfff                          低4bits
    p_adts_header[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    p_adts_header[1] |= (0 << 1);    //Layer:0                                 2bits
    p_adts_header[1] |= 1;           //protection absent:1                     1bit

    p_adts_header[2] = (profile)<<6;            //profile:profile               2bits
    p_adts_header[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    p_adts_header[2] |= (0 << 1);             //private bit:0                   1bit
    p_adts_header[2] |= (channels & 0x04)>>2; //channel configuration:channels  高1bit

    p_adts_header[3] = (channels & 0x03)<<6; //channel configuration:channels 低2bits
    p_adts_header[3] |= (0 << 5);               //original：0                1bit
    p_adts_header[3] |= (0 << 4);               //home：0                    1bit
    p_adts_header[3] |= (0 << 3);               //copyright id bit：0        1bit
    p_adts_header[3] |= (0 << 2);               //copyright id start：0      1bit
    p_adts_header[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    p_adts_header[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    p_adts_header[6] = 0xfc;      //‭11111100‬       //buffer fullness:0x7ff 低6bits
    // number_of_raw_data_blocks_in_frame：
    //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。

    return 0;
}

void ADTS_Header(){
    int ret = -1;
    char errors[1024];
    char *filename = "20--02-FFMPEG如何查询命令帮助文档.mp4";
    char *aacfile = "out.aac";

    FILE *aac_fd = NULL;
    int audio_index = -1;
    int len = 0;
    AVFormatContext *ifmt_ctx = NULL;
    AVPacket pkt;

    aac_fd = fopen(aacfile,"wb");

    ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL);
    if(ret < 0){
        av_strerror(ret, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG,"Could not open source file: %s, %d(%s)\n",
                filename,
                ret,
                errors);
    }
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if(ret < 0){
        av_strerror(ret, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG,"failed to find stream information: %s, %d(%s)\n",
                filename,
                ret,
                errors);
        return;
    }
    av_dump_format(ifmt_ctx, 0, filename, 0);
    av_init_packet(&pkt);
    audio_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_index < 0){
        av_log(NULL,AV_LOG_DEBUG,"Could not find stream\n");
        return;
    }

    printf("Audio profile: %d, FF_PROFILE_AAC_LOW: %d\n",ifmt_ctx->streams[audio_index]->codecpar->profile,
           FF_PROFILE_AAC_LOW);

    if(ifmt_ctx->streams[audio_index]->codecpar->codec_id != AV_CODEC_ID_AAC)
    {
        printf("The media file no contain AAC stream, it's codec_id is %d\n",
               ifmt_ctx->streams[audio_index]->codecpar->codec_id);
        return;
    }

    while(av_read_frame(ifmt_ctx,&pkt)>= 0){
        if(pkt.stream_index == audio_index){
            char adts_header_buf[7] = {0};
            adts_header(adts_header_buf, pkt.size,
                        ifmt_ctx->streams[audio_index]->codecpar->profile,
                        ifmt_ctx->streams[audio_index]->codecpar->sample_rate,
                        ifmt_ctx->streams[audio_index]->codecpar->ch_layout.nb_channels);
            fwrite(adts_header_buf,1,7,aac_fd);
            len = fwrite(pkt.data, 1,pkt.size, aac_fd);
            if(len != pkt.size){
                av_log(NULL,AV_LOG_DEBUG,"Warning,write error (%d:%d)\n",len,pkt.size);
            }
            printf("Write dataSize: %d\n",pkt.size);
        }
        av_packet_unref(&pkt);
    }
    if(ifmt_ctx)
    {
        avformat_close_input(&ifmt_ctx);
    }
    if(aac_fd)
    {
        fclose(aac_fd);
    }
    return;
}

// 解码MP4文件生成对应的h264码流并输出到文件中
void Extract_h264(const char *input_filename, const char *output_filename) {
    AVFormatContext *_avFmtCtx = NULL;
    AVPacket *_avPacket = av_packet_alloc();
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open output file %s\n", output_filename);
        return;
    }

    // 打开输入文件
    if (avformat_open_input(&_avFmtCtx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input file %s\n", input_filename);
        fclose(output_file);
        return;
    }
    // 查找流信息
    if (avformat_find_stream_info(_avFmtCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&_avFmtCtx);
        fclose(output_file);
        return;
    }
    int _videoStreamIndex = -1;
    for (unsigned int i = 0; i < _avFmtCtx->nb_streams; i++) {
        if (_avFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            _videoStreamIndex = i;
            break;
        }
    }
    if (_videoStreamIndex == -1) {
        fprintf(stderr, "Could not find video stream\n");
        avformat_close_input(&_avFmtCtx);
        fclose(output_file);
        return;
    }
    AVPacket spsPacket, ppsPacket, tmpPacket;
    uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t sendSpsPps = 0;

    while (av_read_frame(_avFmtCtx, _avPacket) == 0) {
        // 根据pkt->stream_index判断是不是视频流
        if (_avPacket->stream_index == _videoStreamIndex) {
            // 仅1次处理sps pps
            if (!sendSpsPps) {
                int spsLength = 0;
                int ppsLength = 0;
                uint8_t *ex = _avFmtCtx->streams[_videoStreamIndex]->codecpar->extradata;

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
            uint8_t *data = _avPacket->data;
            while (data < _avPacket->data + _avPacket->size) {
                nalLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                if (nalLength > 0) {
                    memcpy(data, startCode, 4);  // 添加起始码
                    tmpPacket = *_avPacket;      // 复制packet
                    tmpPacket.data = data;		 // 设置指针偏移
                    tmpPacket.size = nalLength + 4; // 更新大小

                    fwrite(tmpPacket.data, 1, tmpPacket.size, output_file); // 写入NALU到文件
                }
                data += 4 + nalLength; // 移动到下一个NALU
            }
        }
        av_packet_unref(_avPacket);
    }

    // 清理资源
    fclose(output_file);
    av_packet_free(&_avPacket);
    avformat_close_input(&_avFmtCtx);
    printf("Finish\n");
}

void DecodeMP4(const char *filename){
    FILE *aac_fd = NULL;
    FILE *h264_fd = NULL;

    h264_fd = fopen("out.h264","wb");
    if(!h264_fd){
        printf("Fopen out.h264 failed\n");
        return;
    }
    aac_fd = fopen("out.aac","wb");
    if(!aac_fd){
        printf("fopen out.aac failed\n");
        return;
    }
    AVFormatContext *fmt_ctx = NULL;
    int videoIndex = -1;
    int audioIndex = -1;
    AVPacket *pkt = NULL;
    int ret = 0;
    char errors[1024];
    fmt_ctx = avformat_alloc_context();
    if(!fmt_ctx){
        printf("avformat_alloc_context failed\n");
        return;
    }
    ret = avformat_open_input(&fmt_ctx,filename,NULL,NULL);
    if(ret < 0){
        av_strerror(ret , errors, 1023);
        printf("avformat_open_input:%s\n",errors);
        avformat_close_input(&fmt_ctx);
        fclose(aac_fd);
        fclose(h264_fd);
        return;
    }

    videoIndex = av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoIndex == -1){
        printf("find video stream errors\n");
        avformat_close_input(&fmt_ctx);
        fclose(aac_fd);
        fclose(h264_fd);
        return;
    }

    audioIndex = av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(videoIndex == -1){
        printf("find audio stream errors\n");
        avformat_close_input(&fmt_ctx);
        fclose(aac_fd);
        fclose(h264_fd);
        return;
    }
    pkt = av_packet_alloc();
    av_init_packet(pkt);

    while(1){
        ret = av_read_frame(fmt_ctx, pkt);
        if(ret < 0){
            av_strerror(ret,errors,1023);
            printf("av_read_frame failed:%s\n",errors);
            break;
        }
        // 处理视频数据
        if(pkt->stream_index == videoIndex){

        }// 处理音频数据
        else if(pkt->stream_index == audioIndex){

        }
        av_packet_unref(pkt);
    }
    if(h264_fd){
        fclose(h264_fd);
    }
    if(aac_fd){
        fclose(aac_fd);
    }
    av_packet_free(&pkt);
    if(fmt_ctx){
        avformat_close_input(&fmt_ctx);
    }
    printf("Finish\n");
    return;
};

void Dexumer(){
    printf("Begin\n");
    const char *filename = "test.mp4";

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

