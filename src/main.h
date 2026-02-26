#pragma once

#define AUDIO_BUFFERS 128
#define AUDIO_THRESHOLD 8
#define AUDIO_DATA_LENGTH 8192

typedef struct buffer_t {
    struct buffer_t *next;
    // The samples are signed 16-bit integers, but ao_play requires a char buffer.
    char data[AUDIO_DATA_LENGTH];
} audio_buffer_t;

typedef struct {
    float freq;
    int mode;
#ifdef USE_RTLSDR
    float gain;
    unsigned int device_index;
    int bias_tee;
    int direct_sampling;
    int ppm_error;
    char *rtltcp_host;
#elif defined USE_SDRPLAY
    float gain;
    char *device_serial;
    int bias_tee;
    int ppm_error;
    char *antenna;
#elif defined USE_SOAPY
    char *gain_settings;
    char *device_args;
    int bias_tee;
    int ppm_error;
    char *antenna;
#endif
    char *input_name;
    ao_device *dev;
    FILE *hdc_file;
    FILE *iq_file;
    char *aas_files_path;

    audio_buffer_t *head, *tail, *free;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    unsigned int program;
    unsigned int audio_ready;
    unsigned int audio_packets_valid;
    unsigned int audio_packets;
    unsigned int audio_bytes;
    unsigned int audio_errors;
    int done;
} state_t;

ao_device *open_ao_live(void);
ao_device *open_ao_file(const char *name, const char *type);
void init_audio_buffers(state_t *st);
void callback(const nrsc5_event_t *evt, void *opaque);
void *input_main(void *arg);
void log_lock(void *udata, int lock);
void cleanup(state_t *st);
