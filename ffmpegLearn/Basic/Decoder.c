#include "Decoder.h"

static char err_buf[128] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}
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
    const char *filename = "20--02-FFMPEG如何查询命令帮助文档.mp4";
    const char *aacfile = "out.aac";
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

// 提取MP4文件的视频流信息，将其解码生成yuv数据并输出到out.yuv
void DecodeMP4toYUV(const char *filename){
    AVFormatContext *fmt_ctx = NULL;
    int videoStream;
    uint32_t i;
    FILE *out = fopen("out.yuv","wb+");
    AVCodecContext *CodecCtx = NULL;

    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket pkt;
    AVCodecParameters *Parameters;

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
    while(av_read_frame(fmt_ctx,&pkt) >= 0){
        if(pkt.stream_index == videoStream){
            //收到了视频数据，开始进行解码处理，生成一帧的YUV图像
            if((ret = avcodec_send_packet(CodecCtx,&pkt)) >= 0){
                // 并不会每次send_packet就会立刻能receive_frame，可能需要多次send才能Receive
                while(avcodec_receive_frame(CodecCtx,frame) >= 0){
                    // 将 YUV 数据写入文件
                    //1080*1920
                    int y_size = CodecCtx->width * CodecCtx->height;
                    int uv_size = (CodecCtx->width / 2) * (CodecCtx->height / 2);

                    printf("Receive one frame y_size: %d  uv_size: %d\n",y_size,uv_size);
                    fwrite(frame->data[0], 1, y_size, out); // Y 平面
                    fwrite(frame->data[1], 1, uv_size, out); // U 平面
                    fwrite(frame->data[2], 1, uv_size, out); // V 平面
                }
            }
            else{
                fprintf(stderr,"avcodec_receive_frame: %s\n",av_get_err(ret));
            }
        }
        av_packet_unref(&pkt);
    }
    printf("Finish w: %d   h: %d\n",CodecCtx->width,CodecCtx->height);
    avformat_close_input(&fmt_ctx);
    avcodec_close(CodecCtx);
    av_packet_unref(&pkt);
    return;
}

void DecodeMP4toPCM(const char *filename) {
    AVFormatContext *fmt_ctx = NULL;
    int audioStream = -1, i;
    AVCodecContext *CodecCtx = NULL;
    const AVCodec *Codec = NULL;
    AVFrame *Frame = NULL;
    AVPacket *Pkt = NULL;
    FILE *out = fopen("out.pcm","wb");
    // 打开输入文件
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input file\n");
        return;
    }

    // 查找流信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream info\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 查找音频流
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            break;
        }
    }
    if (audioStream == -1) {
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 查找解码器
    Codec = avcodec_find_decoder(fmt_ctx->streams[audioStream]->codecpar->codec_id);
    if (Codec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find decoder\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 分配解码器上下文
    CodecCtx = avcodec_alloc_context3(Codec);
    if (CodecCtx == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return;
    }

    // 将流的编解码参数复制到上下文中
    if (avcodec_parameters_to_context(CodecCtx, fmt_ctx->streams[audioStream]->codecpar) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to context\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    // 打开解码器
    if (avcodec_open2(CodecCtx, Codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open codec\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    Frame = av_frame_alloc();
    if (Frame == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate frame\n");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }

    Pkt = av_packet_alloc();
    if (Pkt == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate packet\n");
        av_frame_free(&Frame);
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&CodecCtx);
        return;
    }
    int ret;
    int data_size;
    while (av_read_frame(fmt_ctx, Pkt) >= 0) {
        if (Pkt->stream_index == audioStream) {
            if ((ret = avcodec_send_packet(CodecCtx, Pkt)) >= 0) {
                while (avcodec_receive_frame(CodecCtx, Frame) >= 0) {
                    data_size = av_get_bytes_per_sample(CodecCtx->sample_fmt);
                    if(data_size<0){
                        return;
                    }
                    for(i = 0; i<Frame->nb_samples;i++){
                        fwrite(Frame->data[ret] + data_size*i,1,data_size,out);
                    }
                }
            } else {
                if(ret == AVERROR(EAGAIN)){
                    fprintf(stderr,"Receive_frame and send_packet both \n");
                }
            }
        }
        av_packet_unref(Pkt);
    }
    av_dump_format(fmt_ctx,0,filename,0);
    printf("Finish\n");
    fclose(out);
    // 释放资源
    av_packet_free(&Pkt);
    av_frame_free(&Frame);
    avcodec_free_context(&CodecCtx);
    avformat_close_input(&fmt_ctx);
}
