#ifndef CCOPENGLWIDGET_H
#define CCOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

#define MAX_AUDIO_FRAME_IN_QUEUE    1200
#define MAX_VIDEO_FRAME_IN_QUEUE    600

typedef struct H264FrameDef
{
    unsigned int    length;
    unsigned char*  dataBuffer;

}H264Frame;

typedef struct  H264YUVDef
{
    unsigned int    width;
    unsigned int    height;
    H264Frame       luma;
    H264Frame       chromaB;
    H264Frame       chromaR;
    long long       pts;

}H264YUV_Frame;

typedef struct DecodedAudiodataDef
{
    unsigned char*  dataBuff;
    unsigned int    dataLength;
}JCDecodedAudioData;

enum {
    ATTRIBUTE_VERTEX,
    ATTRIBUTE_TEXCOORD
};
class CCOpenGLWidget:  public QOpenGLWidget,protected QOpenGLFunctions
{
public:
    CCOpenGLWidget(QWidget *parent = 0);
    ~CCOpenGLWidget();

    void RendVideo(H264YUV_Frame * frame);


protected:
    void initializeGL() ;
    void resizeGL(int w, int h) ;
    void paintGL();


private:
    void initializeGLSLShaders();
    GLuint createImageTextures(QString &pathString);

private:

    bool   m_bUpdateData = false;

    GLuint          m_textures[3];


    QOpenGLShaderProgram *m_pShaderProgram = NULL;

    uint8_t m_nVideoW       =0; //视频分辨率宽
    uint8_t m_nVideoH       =0; //视频分辨率高
    uint8_t m_yFrameLength  =0;
    uint8_t m_uFrameLength  =0;
    uint8_t m_vFrameLength  =0;

    unsigned char* m_pBufYuv420p = NULL;

    struct CCVertex{
        float x,y,z;
        float u,v;
    };


};

#endif // CCOPENGLWIDGET_H
