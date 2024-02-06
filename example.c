#include <unistd.h>
#include "emu_decoder.h"
#include "264.c"

static int32_t decode_result(int32_t type, uint8_t *data[], int linesize[])
{
    printf("decode success\n");
    return 0;
}

int main()
{
    decoder_handle handle;
    int i;
    handle = emu_decoder_init();
    emu_decoder_set_decode_proc(handle, decode_result);
    for(i=0; i<sizeof(video_libx264_480_800)/sizeof(video_libx264_480_800[0]);i++) {
        emu_decoder_push_frame(handle, (uint8_t*)video_libx264_480_800[i].data, video_libx264_480_800[i].data_len);
        usleep(1000*50);
    }
    
    return 0;
}