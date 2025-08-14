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

void SaveGreyFramePPM(uint8_t *pixels, int wrap, int height, int width, char* filename) {
    FILE *fp = fopen(filename, "w");
    printf("\n\nWrap: %d\n\n", wrap);
    fprintf(fp, "P5\n%d %d\n%d\n", width, height, 255);
    for (int i = 0; i < height; i++) {
        unsigned char *ch = (pixels + i * wrap);
        fwrite(ch, 1, width, fp);
    }
    fclose(fp);
}

int DecodeVideoPacket_GreyFrame(AVPacket *packet, AVCodecContext *codecContext, AVFrame *frame) {
    int returnValue = 0;

    returnValue = avcodec_send_packet(codecContext, packet);
    if (returnValue != 0) {
        av_log(NULL, AV_LOG_ERROR, "Error decompressing packet\n");
        return returnValue;
    }
    while (returnValue >= 0) {
        returnValue = avcodec_receive_frame(codecContext, frame);
        if (returnValue == AVERROR(EAGAIN)) {
            printf("Not enough data\n");
            av_frame_unref(frame);
            av_freep(frame);
            break;
        } else if (returnValue == AVERROR_EOF) {
            av_log(NULL, AV_LOG_ERROR, "End of File reached\n");
            av_frame_unref(frame);
            av_freep(frame);
            return returnValue;
        } else if (returnValue < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error in receiving frame\n");
            av_frame_unref(frame);
            av_freep(frame);
            return returnValue;
        } else {
            printf("Frame number: %d (type=%c frame, size = %d bytes, width = %d, height = %d) pts %ld key_frame %d [DTS %d]\n",
                   codecContext->frame_number, av_get_picture_type_char(frame->pict_type), frame->pkt_size, frame->width, frame->height, frame->pts,
                   frame->key_frame, frame->coded_picture_number);
            SaveGreyFramePPM(frame->data[0], frame->linesize[0], frame->height, frame->width, "Test.ppm");
        }
    }

    return returnValue;
}

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
    if (videoStreamIndex == -1) {
        av_log(NULL, AV_LOG_ERROR, "Error finding video stream\n");
        return -1;
    }

    videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating codec context\n");
        return -1;
    }

    returnValue = avcodec_parameters_to_context(videoCodecContext, videoCodecParameters);
    if (returnValue != 0) {
        av_log(NULL, AV_LOG_ERROR, "Error copying codec parameters to context\n");
        return -1;
    }

    returnValue = avcodec_open2(videoCodecContext, videoCodec, NULL);
    if (returnValue != 0) {
        av_log(NULL, AV_LOG_ERROR, "Error opening avcodec\n");
        return -1;
    }

    int packetCount = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            int64_t duration = packet->pts;
            //printf("Video packet: \n");
            //FormatDuration(duration);
            returnValue = DecodeVideoPacket_GreyFrame(packet, videoCodecContext, videoFrame);
        } else if (packet->stream_index == audioStreamIndex) {
            int64_t duration = packet->pts;
            //printf("Audio packet: \n");
            //FormatDuration(duration);
        }
        packetCount += 1;
        av_packet_unref(packet);
        //if (packetCount == 10) {
         //   break;
       // }
    }

    sws_freeContext(swsContext);
    avformat_close_input(&formatContext);
    av_packet_free(&packet);
    av_free(videoFrame);
    avcodec_free_context(&videoCodecContext);
    return 0;
}
