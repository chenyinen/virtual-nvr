#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <queue/queue.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "emu_decoder.h"

struct emu_decoder_t {
    bool running;
    AVCodecContext *codecContext;
    AVFrame *decode_frame;
    queue_t *decode_que;
    int32_t (*decode_result)(int32_t type, uint8_t *data[], int linesize[]);
};

struct emu_decoder_packet_t {
    AVPacket *packet;
    uint8_t *data;
};

static void decoder_free_pack(struct emu_decoder_packet_t *decoder_packet)
{
    if (decoder_packet) {
        av_packet_free(&decoder_packet->packet);
        free(decoder_packet->data);
        free(decoder_packet);
    }
}
void emu_decoder_set_decode_proc(decoder_handle handle, int32_t (*decode_result)(int32_t stream_type, uint8_t *data[], int linesize[]))
{
    struct emu_decoder_t *emu_decoder_ctx = (struct emu_decoder_t *)handle;

    emu_decoder_ctx->decode_result = decode_result;
}
static void* decode_proc(void *p)
{
    struct emu_decoder_t *emu_decoder_ctx = p;
    struct emu_decoder_packet_t *decoder_packet;
    AVFrame *decode_frame = emu_decoder_ctx->decode_frame;
    int32_t ret;
    void *element;

    while(emu_decoder_ctx->running) {
        queue_get_wait(emu_decoder_ctx->decode_que, &element);
        decoder_packet = (struct emu_decoder_packet_t *)element;
        ret = avcodec_send_packet(emu_decoder_ctx->codecContext, decoder_packet->packet);
        if (ret < 0) {
            printf("av send pack fail\n");
            decoder_free_pack(decoder_packet);
            continue;
        }
        ret = avcodec_receive_frame(emu_decoder_ctx->codecContext, decode_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            decoder_free_pack(decoder_packet);
            continue;
        }
        else if (ret < 0) {
            printf("avcodec receive frame fail: %s\n", av_err2str(ret));
            continue;
        }
        if (emu_decoder_ctx->decode_result) {
            emu_decoder_ctx->decode_result(decode_frame->format,  decode_frame->data, decode_frame->linesize);
        }
        decoder_free_pack(decoder_packet);
        av_frame_unref(decode_frame);
    }

}
int32_t emu_decoder_push_frame(decoder_handle handle, uint8_t *data, int32_t size)
{
    AVPacket *packet;
    struct emu_decoder_packet_t *decoder_packet;
    struct emu_decoder_t *emu_decoder_ctx = (struct emu_decoder_t *)handle;
    int32_t ret;

    decoder_packet = calloc(1, sizeof(*decoder_packet));
    if (!decoder_packet) {
        return -1;
    }
    decoder_packet->data = malloc(size);
    if (!decoder_packet->data) {
        free(decoder_packet);
        return -1;
    }
    memcpy(decoder_packet->data, data, size);
    packet = av_packet_alloc();
    if (!packet) {
        free(decoder_packet->data);
        free(decoder_packet);
        return -1;
    }
    av_init_packet(packet);
    packet->data =  decoder_packet->data;
    packet->size = size;
    decoder_packet->packet = packet;
    ret = queue_put(emu_decoder_ctx->decode_que, (void*)decoder_packet);
    if (Q_OK != ret) {
        decoder_free_pack(decoder_packet);
        return ret;
    }
    return 0;
}
decoder_handle emu_decoder_init()
{
    int32_t ret;
    pthread_attr_t attr;
    pthread_t pthread_id;
    AVCodecContext *codecContext = NULL;
    AVCodec *codec = NULL;
    queue_t *que = NULL;
    struct emu_decoder_t *emu_decoder_ctx = NULL;

    avcodec_register_all();

    emu_decoder_ctx = calloc(1, sizeof(*emu_decoder_ctx));
    if (!emu_decoder_ctx) {
        goto cleanup;
    }
    que = queue_create_limited(5);
    if (!que) {
        goto cleanup;
    }
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        printf("avcodec can't found decoder\n");
        goto cleanup;
    }
    codecContext = avcodec_alloc_context3(NULL);
    if (!codecContext) {
        goto cleanup;
    }
    ret = avcodec_open2(codecContext, codec, NULL);
    if (ret < 0) {
        goto cleanup;
    }
    emu_decoder_ctx->decode_frame = av_frame_alloc();
    if (!emu_decoder_ctx->decode_frame) {
        goto cleanup;
    }
    
    emu_decoder_ctx->decode_que = que;
    emu_decoder_ctx->codecContext = codecContext;
    emu_decoder_ctx->running = true;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pthread_id, &attr, decode_proc, emu_decoder_ctx);

    return emu_decoder_ctx;
cleanup:
    printf("emu init fail\n");
    if (emu_decoder_ctx) {
        free(emu_decoder_ctx);
    }
    if(que) {
        queue_destroy(que);
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (emu_decoder_ctx->decode_frame) {
        av_frame_free(&emu_decoder_ctx->decode_frame);
    }
    return NULL;
}