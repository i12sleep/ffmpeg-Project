#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    char *fileName = argv[1];
    printf("File to process: %s\n", argv[1]);

    AVFormatContext *formatContext = NULL;

    AVPacket *packet = NULL;
    AVFrame *videoFrame = NULL;
    
    AVCodecParameters *videoCodecParameters = NULL;
    AVCodec *videoCodec = NULL;
    AVCodecContext *videoCodecContext = NULL;
    int videoStreamIndex = -1;

    struct SwsContext *swsContext = NULL;

    AVCodecParameters *audioCodecParameters = NULL;
    AVCodec *audioCodec = NULL;
    AVCodecContext *audioCodecContext = NULL;
    int audioStreamIndex = -1;

    int returnValue = 0;

    // check for file and allocate format context
    returnValue = avformat_open_input(&formatContext, fileName, NULL, NULL);
    if (returnValue != 0) {
        av_log(NULL, AV_LOG_ERROR, "Error opening file\n");
        return -1;
    }

    // allocate packet
    packet = av_packet_alloc();
    if (!packet) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating packet\n");
        return -1;
    }

    videoFrame = av_frame_alloc();
    if (!videoFrame) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating video frame\n");
        return -1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Stream information not found\n");
        return -1;
    }

    for (int i = 0; i < formatContext->nb_streams; i++) {
        AVCodecParameters *currentCodecParameters = NULL;
        AVCodec *currentCodec = NULL;
        AVStream *currentStream = NULL;

        currentStream = formatContext->streams[i];
        currentCodecParameters = currentStream->codecpar;
        currentCodec = avcodec_find_decoder(currentCodecParameters->codec_id);
        if (currentCodec == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Codec not supported\n");
            continue;
        }

        if (currentCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            double frameRate = av_q2d(currentStream->r_frame_rate);
            videoStreamIndex = i;
            videoCodec = currentCodec;
            videoCodecParameters = currentCodecParameters;

            printf("\nFound video stream\n");
            printf("ID: %d\n Codec: %s\n BitRate: %ld\n Width: %d, Height: %d\nFramerate: %f fps\n",
                    currentCodecParameters->codec_id, currentCodec->name, 
                    currentCodecParameters->bit_rate, currentCodecParameters->width, 
                    currentCodecParameters->height, frameRate);
        } else if (currentCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            audioCodec = currentCodec;
            audioCodecParameters = currentCodecParameters;
            printf("\nFound audio stream\n");
            printf("ID: %d\n Codec: %s\n BitRate: %ld\n Channels: %d, Sample Rate: %d\n", 
                currentCodecParameters->codec_id, currentCodec->name, 
                currentCodecParameters->bit_rate, currentCodecParameters->channels, 
                currentCodecParameters->sample_rate);           
        } else if (currentCodecParameters->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            printf("\nFound subtitle stream\n");
        }
    }

    return 0;
}
