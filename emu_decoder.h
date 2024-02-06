#ifndef EMU_DECODER__H_
#define EMU_DECODER__H_

#include <stdio.h>
#include <stdint.h>

typedef struct emu_decoder_t* decoder_handle;

decoder_handle emu_decoder_init();
int32_t emu_decoder_push_frame(decoder_handle handle, uint8_t *data, int32_t size);
void emu_decoder_set_decode_proc(decoder_handle handle, int32_t (*decode_result)(int32_t stream_type, uint8_t *data[], int linesize[]));

#endif