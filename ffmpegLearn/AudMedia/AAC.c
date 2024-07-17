#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libavutil/frame.h>
#include <libavutil/mem.h>

#include <libavcodec/avcodec.h>

#define AUDIO_INBUF_SIZE 20480
#define ADUIO_REFILL_THRESH 4096

static char err_buf[128] = {0};
static char *av_get_err(int errnum){
    av_strerror(errnum,err_buf,128);
    return err_buf;
}

static void print_sample_format(const AVFrame *frame){
    printf("ar-samplerate: %uHz\n",frame->sample_rate);
    printf("ac-channel: %u\n",frame->channels);
    printf("f-format: %u\n",frame->format);
}

static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                    int ofd)
{
    int i,ch;
    int ret, data_size;
    ret = avcodec_send_packet(dec_ctx, pkt);
    if(ret == AVERROR(EAGAIN)){
        fprintf(stderr,"Receive_frame and send_packet both returned EAGIN, which is an API violation\n");
    }
    else if(ret < 0){
        fprintf(stderr, "Error submitting the packet to the decoder, err:%s, pkt_size:%d\n",
            av_get_err(ret), pkt->size);
        return;
    }

    while(ret >= 0){
        ret = avcodec_receive_frame(dec_ctx, frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if(ret < 0){
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if(data_size < 0){
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        static int s_printf_format = 0;
        if(s_printf_format == 0){
            s_printf_format = 1;
            print_sample_format(frame);
        }

        for(i = 0;i < frame->nb_samples;i++){
            for(ch = 0;ch<dec_ctx->channels; ch++)
                write(ofd,frame->data[ch] + data_size*i,data_size);
        }
    }
}

int main(int argc,char *argv[]){
    const char *outfilename;
    const char *filename;
    const AVCodec *codec;
    AVCodecContext *codec_ctx= NULL;
    AVCodecParserContext *parser = NULL;
    int len = 0;
    int ret = 0;
    int len = 0;
    int ret = 0;
    int ifd,ofd;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data = NULL;
    size_t data_size = 0;
    AVPacket *pkt = NULL;
    AVFrame *decoded_frame = NULL;

    if(argc <= 2){
        fprintf(stderr, "Usage: %s error",argv[0]);
        exit(0);
    }
    filename = argv[1];
    outfilename = argv[2];
    pkt = av_packet_alloc();
    enum AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
    if(strstr(inputfile, "aac") != NULL){
        audio_codec_id = AV_CODEC_ID_AAC;
    }
    else if (strstr(inputfile,"mp3")!=NULL){
        audio_codec_id = AV_CODEC_ID_MP3;
    }
    else{
        printf("default codec id:%d\n",audio_codec_id);
    }

    codec = avcodec_find_decoder(audio_codec_id);
    if(!codec){
        fprintf(stderr,"Codec not find\n");
        exit(1);
    }
    parser = av_parser_init(codec->id);
    if(!parser){
        fprintf(stderr,"Parser not find");
        exit(1);
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if(codec_ctx){
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    if(avcodec_open2(codec_ctx, codec, NULL) < 0){
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    ifd = open(filename, O_RDONLY);
    if(ifd < 0){
        fprintf(stderr, "Could Not open %s\n",filename);
        exit(1);
    }
    ofd = open(filename, O_RDWR);
    if(ofd < 0){
        av_free(codec_ctx);
        close(ifd);
        exit(1);
    }
    data = inbuf;
    data_size = read(ifd,data,AUDIO_INBUF_SIZE);

    while(data_size > 0){
        if(!decoded_frame){
            if(!(decoded_frame = av_frame_alloc())){
                fprintf(stderr,"Could not allocate audio frame\n");
                exit(1);
            }
        }
        ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size,
                                data, data_size,
                                AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if(ret < 0){
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        data += ret;
        data_size -= ret;

        if(pkt->size)
            decode(codec_ctx, pkt, decoded_frame, ofd);
        if(data_size < ADUIO_REFILL_THRESH)
        {
            memmove(inbuf,data,data_size);
            data = inbuf;

            len = read(ifd,data+data_size,AUDIO_INBUF_SIZE - data_size);
            if(len > 0)
                data_size += len;
        }
    }

     /* 冲刷解码器 */
    pkt->data = NULL;   // 让其进入drain mode
    pkt->size = 0;
    decode(codec_ctx, pkt, decoded_frame, outfile);

    close(ofd);
    close(ifd);

    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    printf("main finish, please enter Enter and exit\n");
    return 0;
}