#include <stdlib.h>
#include <stdint.h>
#include "../inc/logger.h"
#include "../inc/devices.h"
#include "../inc/command.h"
#include "../inc/audio.h"
#include "../inc/helpers.h"
#define USE_POCKETSPHINX

#ifdef USE_POCKETSPHINX
#include <pocketsphinx.h>
#endif


struct {
    uint8_t is_enabled;
    uint8_t stay_running;
} speech_to_text;


void speech_to_text_stay_running(uint8_t enable) {
    logger(MSG_INFO, "%s: SET %u\n", __func__, enable);
    speech_to_text.stay_running = enable;
}
#ifdef USE_POCKETSPHINX

int
callaudio_stt_demo(char *filename) {
    ps_decoder_t *decoder;
    ps_config_t *config;
    FILE *fh;
    short *buf;
    size_t len, nsamples;

    if (speech_to_text.is_enabled) {
        logger(MSG_ERROR, "%s: Already running\n", __func__);
        return 0;
    }

    logger(MSG_INFO,"%s: Start\n", __func__);
    speech_to_text.is_enabled = 1;

    /* Look for a single audio file as input parameter. */
    if ((fh = fopen(filename, "rb")) == NULL) {
        logger(MSG_ERROR,"Failed to open %s", filename);
        speech_to_text.is_enabled = 0;
        return -1;
    }

    /* Get the size of the input. */
    if (fseek(fh, 0, SEEK_END) < 0)
        logger(MSG_ERROR,"Unable to find end of input file %s", filename);
    len = ftell(fh);
    rewind(fh);

    /* Initialize configuration from input file. */
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if (ps_config_soundfile(config, fh, filename) < 0) {
        logger(MSG_ERROR,"Unsupported input file %s\n", filename);
        if (config)
            free(config);
        speech_to_text.is_enabled = 0;

        return -1;
    }
    if ((decoder = ps_init(config)) == NULL) {
        logger(MSG_ERROR,"PocketSphinx decoder init failed\n");
        if (config)
            free(config);
        speech_to_text.is_enabled = 0;

        return -1;
    }

    /* Allocate data (skipping header) */
    len -= ftell(fh);
    if ((buf = malloc(len)) == NULL)
        logger(MSG_ERROR,"Unable to allocate %d bytes", len);
    /* Read input */
    nsamples = fread(buf, sizeof(buf[0]), len / sizeof(buf[0]), fh);
    if (nsamples != len / sizeof(buf[0]))
        logger(MSG_ERROR,"Unable to read %d samples", len / sizeof(buf[0]));

    /* Recognize it! */
    if (ps_start_utt(decoder) < 0)
        logger(MSG_ERROR,"Failed to start processing\n");
    if (ps_process_raw(decoder, buf, nsamples, FALSE, TRUE) < 0)
        logger(MSG_ERROR,"ps_process_raw() failed\n");
    if (ps_end_utt(decoder) < 0)
        logger(MSG_ERROR,"Failed to end processing\n");

    /* Print the result */
    if (ps_get_hyp(decoder, NULL) != NULL)
        logger(MSG_INFO, "%s: %s\n",__func__, ps_get_hyp(decoder, NULL));

    /* Clean up */
    if (fclose(fh) < 0)
        logger(MSG_ERROR,"Failed to close %s", filename);
    free(buf);
    ps_free(decoder);
    ps_config_free(config);
    speech_to_text.is_enabled = 0;

    return 0;
}

/* launched from call thread */
void *callaudio_stt_continuous()
{
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    short *frame;
    size_t frame_size;
    struct pcm *incall_pcm_rx;
    size_t bufsize;
    logger(MSG_INFO, "%s: START\n", __func__);
    if (speech_to_text.is_enabled) {
        logger(MSG_ERROR, "%s: Already running\n", __func__);
        return NULL;
    }

    if(!speech_to_text.stay_running) {
        logger(MSG_ERROR, "%s: Wasn't ordered to stay running\n", __func__);
        return NULL;
    }

  incall_pcm_rx = pcm_open((PCM_IN | PCM_MONO | PCM_MMAP), PCM_DEV_HIFI);
  if (incall_pcm_rx == NULL) {
    logger(MSG_INFO, "%s: Error opening %s (rx), bailing out\n", __func__,
           PCM_DEV_HIFI);
    speech_to_text.stay_running = 0;
    return NULL;
  }
    logger(MSG_INFO, "%s: ARMED\n", __func__);
    speech_to_text.is_enabled = 1;
    incall_pcm_rx->channels = 1;
    incall_pcm_rx->flags = PCM_IN | PCM_MONO;
    incall_pcm_rx->format = PCM_FORMAT_S16_LE;
    incall_pcm_rx->rate = 16000;
    incall_pcm_rx->period_size = 1024;
    incall_pcm_rx->period_cnt = 1;
    incall_pcm_rx->buffer_size = 32768;
    if (set_params(incall_pcm_rx, PCM_IN)) {
        logger(MSG_ERROR, "Error setting RX Params\n");
        speech_to_text.is_enabled = 0;
        speech_to_text.stay_running = 0;
        pcm_close(incall_pcm_rx);
        return NULL;
    }

    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL) {
        logger(MSG_ERROR,"PocketSphinx decoder init failed\n");
        speech_to_text.is_enabled = 0;
        speech_to_text.stay_running = 0;
        pcm_close(incall_pcm_rx);
        return NULL;
    }

    if ((ep = ps_endpointer_init(0, 0.0, 0, 0, 0)) == NULL) {
        logger(MSG_ERROR,"PocketSphinx endpointer init failed\n");
        speech_to_text.is_enabled = 0;
        speech_to_text.stay_running = 0;
        pcm_close(incall_pcm_rx);
        return NULL;
    }
//    sox = popen_sox(ps_endpointer_sample_rate(ep));
    frame_size =  pcm_get_buffer_size(incall_pcm_rx); // ps_endpointer_frame_size(ep);
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        logger(MSG_ERROR,"Failed to allocate frame");

    while (speech_to_text.stay_running) {
        logger(MSG_INFO, "%s: LOOPING\n", __func__);

        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        size_t len, end_samples;
        if (pcm_read(incall_pcm_rx, frame,
            pcm_bytes_to_frames(incall_pcm_rx,
                                pcm_get_buffer_size(incall_pcm_rx))) == 0) {
                speech = ps_endpointer_end_stream(ep, frame,
                                                  frame_size,
                                                  &end_samples);
            logger(MSG_INFO, "%s: Read %u bytes\n", __func__, frame_size);
        } else {
            speech = ps_endpointer_process(ep, frame);
        }
        if (speech != NULL) {
            logget(MSG_INFO, "%s: Speech is not null\n", __func__);
            const char *hyp;
            if (!prev_in_speech) {
                logger(MSG_ERROR, "%s: Speech start at %.2f\n", __func__,
                        ps_endpointer_speech_start(ep));
                ps_start_utt(decoder);
            }
            if (ps_process_raw(decoder, speech, frame_size, FALSE, FALSE) < 0)
                logger(MSG_ERROR,"%s: ps_process_raw() failed\n", __func__);
            if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
                logger(MSG_ERROR, "%s: PARTIAL RESULT: %s\n", __func__, hyp);
                parse_command(hyp);
            }
            if (!ps_endpointer_in_speech(ep)) {
                logger(MSG_ERROR, "%s: Speech end at %.2f\n", __func__,
                        ps_endpointer_speech_end(ep));
                ps_end_utt(decoder);
                if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
                    logger(MSG_INFO, "%s: %s\n", __func__, hyp);
                    parse_command(hyp);


                }
            }
        } else {
            logger(MSG_WARN, "%s: Speech is null!\n", __func__);
        }
    }
    logger(MSG_INFO, "%s: GETTING OUT\n", __func__);

    free(frame);

    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);
    speech_to_text.is_enabled = 0;
    speech_to_text.stay_running = 0;
    pcm_close(incall_pcm_rx);

    return NULL;
}
#else
int
callaudio_stt_demo(char *filename) {
    return 0;
}
void *callaudio_stt_continuous() {
    return NULL;
}
#endif