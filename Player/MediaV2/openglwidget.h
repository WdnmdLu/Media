#ifndef CCOPENGLWIDGET_H
#define CCOPENGLWIDGET_H
#include <QtOpenGLWidgets/QtOpenGLWidgets>
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

    uint m_nVideoW       =0; //视频分辨率宽
    uint m_nVideoH       =0; //视频分辨率高
    uint m_yFrameLength  =0; //y分量的字节数
    uint m_uFrameLength  =0; //u分量的字节数
    uint m_vFrameLength  =0; //v分量的字节数

    unsigned char* m_pBufYuv420p = NULL;
    // x，y，z坐标  u，v坐标
    struct CCVertex{
        float x,y,z;
        float u,v;
    };


};

#endif // CCOPENGLWIDGET_H
