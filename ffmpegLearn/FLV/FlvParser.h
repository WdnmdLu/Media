#ifndef FLVPARSER_H
#define FLVPARSER_H

#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <Videojj.h>

class CFlvParser{
    public:
        CFlvParser();
        virtual ~CFlvParser();

        int Parse(uint8_t *Buf,int BufSize,int UsedLen);

        int PrintInfo();

        int DumpH264();

        int DumpAAC();

        int DumpFlv();

    private:
        typedef struct FlvHeader{
            int Version;
            int haveVideo;
            int haveAudio;
            int headSize;
            uint8_t *FlvHeader; 
        }FlvHeader;

        struct TagHeader{
            int Type;      // Tag类型（视频、音频或元数据）
            int DataSize;  // Tag数据的大小
            int TimeStamp; // Tag的时间戳
            int TSEx;      // 时间戳扩展
            int StreamID;  // 流ID，总是0
            uint32_t TotalTS;  // 完整的时间戳
            TagHeader() : Type(0), DataSize(0), TimeStamp(0), TSEx(0), StreamID(0), TotalTS(0) {}
            ~TagHeader() {}
        }

    // Tag类定义
    class Tag
    {
    public:
        Tag() : _pTagHeader(NULL), _pTagData(NULL), _pMedia(NULL), _nMediaLen(0) {}
        void Init(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen);

        TagHeader _header;
        uint8_t *_pTagHeader;   // 指向标签头部
        uint8_t *_pTagData;     // 指向标签数据
        uint8_t *_pMedia;       // 指向媒体数据
        int _nMediaLen;         // 媒体数据长度
    };

    // 视频Tag类定义，继承自Tag类
    class CVideoTag : public Tag
    {
    public:
        CVideoTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);
        int _nFrameType;    // 视频帧类型
        int _nCodecID;      // 视频编解码器ID
        int ParseH264Tag(CFlvParser *pParser);
        int ParseH264Configuration(CFlvParser *pParser, uint8_t *pTagData);
        int ParseNalu(CFlvParser *pParser, uint8_t *pTagData);
    };

    // 音频Tag类定义，继承自Tag类
    class CAudioTag : public Tag
    {
    public:
        CAudioTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        int _nSoundFormat;  // 音频编码格式
        int _nSoundRate;    // 音频采样率
        int _nSoundSize;    // 音频位深
        int _nSoundType;    // 音频类型

        // AAC音频相关静态成员
        static int _aacProfile;     // AAC配置文件
        static int _sampleRateIndex;    // 采样率索引
        static int _channelConfig;      // 通道配置

        int ParseAACTag(CFlvParser *pParser);
        int ParseAudioSpecificConfig(CFlvParser *pParser, uint8_t *pTagData);
        int ParseRawAAC(CFlvParser *pParser, uint8_t *pTagData);
    };

    // 元数据Tag类定义，继承自Tag类
    class CMetaDataTag : public Tag
    {
    public:
        CMetaDataTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        double hexStr2double(const unsigned char* hex, const unsigned int length);
        int parseMeta(CFlvParser *pParser);
        void printMeta();

        uint8_t m_amf1_type;
        uint32_t m_amf1_size;
        uint8_t m_amf2_type;
        unsigned char *m_meta;
        unsigned int m_length;

        double m_duration;
        double m_width;
        double m_height;
        double m_videodatarate;
        double m_framerate;
        double m_videocodecid;

        double m_audiodatarate;
        double m_audiosamplerate;
        double m_audiosamplesize;
        bool m_stereo;
        double m_audiocodecid;

        string m_major_brand;
        string m_minor_version;
        string m_compatible_brands;
        string m_encoder;

        double m_filesize;
    };

    // FLV统计信息结构体定义
    struct FlvStat
    {
        int nMetaNum, nVideoNum, nAudioNum;
        int nMaxTimeStamp;
        int nLengthSize;

        FlvStat() : nMetaNum(0), nVideoNum(0), nAudioNum(0), nMaxTimeStamp(0), nLengthSize(0){}
        ~FlvStat() {}
    };

    // 静态成员函数，用于显示不同长度的无符号整数
    static uint32_t ShowU32(uint8_t *pBuf) { return (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3]; }
    static uint32_t ShowU24(uint8_t *pBuf) { return (pBuf[0] << 16) | (pBuf[1] << 8) | (pBuf[2]); }
    static uint32_t ShowU16(uint8_t *pBuf) { return (pBuf[0] << 8) | (pBuf[1]); }
    static uint32_t ShowU8(uint8_t *pBuf) { return (pBuf[0]); }

    // 写入不同长度的无符号整数
    static void WriteU64(uint64_t & x, int length, int value)
    {
        uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - length);
        x = (x << length) | ((uint64_t)value & mask);
    }
    static uint32_t WriteU32(uint32_t n)
    {
        uint32_t nn = 0;
        uint8_t *p = (uint8_t *)&n;
        uint8_t *pp = (uint8_t *)&nn;
        pp[0] = p[3];
        pp[1] = p[2];
        pp[2] = p[1];
        pp[3] = p[0];
        return nn;
    }

    // 友元类声明，允许Tag类访问CFlvParser的私有成员
    friend class Tag;

private:
    // 私有成员函数
    FlvHeader *CreateFlvHeader(uint8_t *pBuf);
    int DestroyFlvHeader(FlvHeader *pHeader);
    Tag *CreateTag(uint8_t *pBuf, int nLeftLen);
    int DestroyTag(Tag *pTag);
    int Stat();
    int StatVideo(Tag *pTag);
    int IsUserDataTag(Tag *pTag);

private:
    // 私有成员变量
    FlvHeader* _pFlvHeader; // 指向FLV头部
    std::vector<Tag *> _vpTag;   // 存储所有Tag的向量
    FlvStat _sStat;         // FLV统计信息
    CVideojj *_vjj;         // 指向视频处理类的指针
    int _nNalUnitLength;    // NAL单元长度
};