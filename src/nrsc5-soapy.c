#include <assert.h>
#include <string.h>

#include "private.h"

static int using_worker(nrsc5_t *st)
{
    return st->dev || st->iq_file;
}

// fv - FIXME
#if 0
static void worker_cb(uint8_t *buf, uint32_t len, void *arg)
{
    nrsc5_t *st = arg;

    input_push_cu8(&st->input, buf, len);
}
#endif

static void *worker_thread(void *arg)
{
    nrsc5_t *st = arg;

    pthread_mutex_lock(&st->worker_mutex);
    while (!st->closed)
    {
        if (st->stopped && !st->worker_stopped)
        {
            st->worker_stopped = 1;
            pthread_cond_broadcast(&st->worker_cond);
        }
        else if (!st->stopped && st->worker_stopped)
        {
            st->worker_stopped = 0;
            pthread_cond_broadcast(&st->worker_cond);
        }

        if (st->stopped)
        {
            // wait for a signal
            pthread_cond_wait(&st->worker_cond, &st->worker_mutex);
        }
        else
        {
            int err = 0;

            pthread_mutex_unlock(&st->worker_mutex);

            if (st->dev)
            {
                void *buffs[] = {st->samples_buf}; //array of buffers
                int flags; //flags set by receive operation
                long long timeNs; //timestamp for receive buffer
                err = SoapySDRDevice_readStream(st->dev, st->rx_stream, buffs, 128 * 256 / 2, &flags, &timeNs, 100000);
                if (err >= 0) {
                    input_push_cs16(&st->input, st->samples_buf, err*2);
                    err = 0;
                } else {
                    log_error("SoapySDRDevice_readStream failed");
                }
            }
            else if (st->iq_file)
            {
                int count = fread(st->samples_buf, 4, sizeof(st->samples_buf) / 4, st->iq_file);
                if (count > 0)
                    input_push_cs16(&st->input, st->samples_buf, count * 4);
                if (feof(st->iq_file) || ferror(st->iq_file))
                    err = 1;
            }

            pthread_mutex_lock(&st->worker_mutex);

            if (err)
            {
                st->stopped = 1;
                nrsc5_report_lost_device(st);
            }
        }
    }

    pthread_mutex_unlock(&st->worker_mutex);
    return NULL;
}

static void nrsc5_init(nrsc5_t *st)
{
    st->closed = 0;
    st->stopped = 1;
    st->worker_stopped = 1;
    st->gain_settings[0] = '\0';
    st->freq = NRSC5_SCAN_BEGIN;
    st->mode = NRSC5_MODE_FM;
    st->callback = NULL;

    output_init(&st->output, st);
    input_init(&st->input, st, &st->output);

    if (using_worker(st))
    {
        // Create worker thread
        pthread_mutex_init(&st->worker_mutex, NULL);
        pthread_cond_init(&st->worker_cond, NULL);
        pthread_create(&st->worker, NULL, worker_thread, st);
    }
}

static nrsc5_t *nrsc5_alloc(void)
{
    nrsc5_t *st = calloc(1, sizeof(*st));
    return st;
}

int nrsc5_open(nrsc5_t **result, char *device_args)
{
    int err;
    nrsc5_t *st = nrsc5_alloc();

    if (!(st->dev = SoapySDRDevice_makeStrArgs(device_args)))
        goto error_init;

    err = SoapySDRDevice_setSampleRate(st->dev, SOAPY_SDR_RX, 0, NRSC5_SAMPLE_RATE_CS16_FM);
    if (err) goto error;
    /* increase bandwidth since NRSC5 requires about 400kHz */
    err = SoapySDRDevice_setBandwidth(st->dev, SOAPY_SDR_RX, 0, 600e3);
    if (err) goto error;

    nrsc5_init(st);

    *result = st;
    return 0;

error:
    log_error("nrsc5_open error: %d", err);
    SoapySDRDevice_unmake(st->dev);
error_init:
    free(st);
    *result = NULL;
    return 1;
}

int nrsc5_open_file(nrsc5_t **result, FILE *fp)
{
    nrsc5_t *st = nrsc5_alloc();
    st->iq_file = fp;
    nrsc5_init(st);

    *result = st;
    return 0;
}

int nrsc5_open_pipe(nrsc5_t **result)
{
    nrsc5_t *st = nrsc5_alloc();
    nrsc5_init(st);

    *result = st;
    return 0;
}

void nrsc5_close(nrsc5_t *st)
{
    if (!st)
        return;

    if (using_worker(st))
    {
        // signal the worker to exit
        pthread_mutex_lock(&st->worker_mutex);
        st->closed = 1;
        pthread_cond_broadcast(&st->worker_cond);
        pthread_mutex_unlock(&st->worker_mutex);

        // wait for worker to finish
        pthread_join(st->worker, NULL);
    }

    if (st->dev)
        SoapySDRDevice_unmake(st->dev);
    if (st->iq_file)
        fclose(st->iq_file);

    input_free(&st->input);
    output_free(&st->output);
    free(st);
}

void nrsc5_start(nrsc5_t *st)
{
    if (st->dev)
    {
        st->rx_stream = SoapySDRDevice_setupStream(st->dev, SOAPY_SDR_RX, SOAPY_SDR_CS16, NULL, 0, NULL);
        if (st->rx_stream == NULL) {
            log_error("SoapySDRDevice_setupStream failed");
            return;
        }
        SoapySDRDevice_activateStream(st->dev, st->rx_stream, 0, 0, 0);
    }

    if (using_worker(st))
    {
        // signal the worker to start
        pthread_mutex_lock(&st->worker_mutex);
        st->stopped = 0;
        pthread_cond_broadcast(&st->worker_cond);
        pthread_mutex_unlock(&st->worker_mutex);
    }
}

void nrsc5_stop(nrsc5_t *st)
{
    if (using_worker(st))
    {
        // signal the worker to stop
        pthread_mutex_lock(&st->worker_mutex);
        st->stopped = 1;
        pthread_cond_broadcast(&st->worker_cond);
        pthread_mutex_unlock(&st->worker_mutex);

        // wait for worker to stop
        pthread_mutex_lock(&st->worker_mutex);
        while (st->stopped != st->worker_stopped)
            pthread_cond_wait(&st->worker_cond, &st->worker_mutex);
        pthread_mutex_unlock(&st->worker_mutex);
    }

    if (st->dev)
    {
        SoapySDRDevice_deactivateStream(st->dev, st->rx_stream, 0, 0);
        SoapySDRDevice_closeStream(st->dev, st->rx_stream);
    }
}

int nrsc5_set_mode(nrsc5_t *st, int mode)
{
    if (mode == NRSC5_MODE_FM || mode == NRSC5_MODE_AM)
    {
        st->mode = mode;
        input_set_mode(&st->input);
        return 0;
    }
    return 1;
}

int nrsc5_set_bias_tee(nrsc5_t *st, int on)
{
    if (st->dev)
    {
        int err = SoapySDRDevice_writeSetting(st->dev, "biasT_ctrl", on ? "true" : "false");
        if (err)
            return 1;
    }
    return 0;
}

int nrsc5_set_antenna(nrsc5_t *st, char *antenna)
{
    if (!antenna)
        return 0;

    if (st->dev)
    {
        int err = SoapySDRDevice_setAntenna(st->dev, SOAPY_SDR_RX, 0, antenna);
        if (err)
            return 1;
    }
    return 0;
}

int nrsc5_set_freq_correction(nrsc5_t *st, int ppm_error)
{
    if (st->dev)
    {
        int err = SoapySDRDevice_setFrequencyCorrection(st->dev, SOAPY_SDR_RX, 0, ppm_error);
        if (err)
            return 1;
    }
    return 0;
}

void nrsc5_get_frequency(nrsc5_t *st, float *freq)
{
    if (st->dev)
        *freq = SoapySDRDevice_getFrequency(st->dev, SOAPY_SDR_RX, 0);
    else
        *freq = st->freq;
}

int nrsc5_set_frequency(nrsc5_t *st, float freq)
{
    if (st->freq == freq)
        return 0;
    if (!st->stopped)
        return 1;

    if (st->dev && SoapySDRDevice_setFrequency(st->dev, SOAPY_SDR_RX, 0, freq, NULL) != 0)
        return 1;

    input_reset(&st->input);
    output_reset(&st->output);

    st->freq = freq;
    return 0;
}

void nrsc5_get_gain(nrsc5_t *st, char **gain_settings)
{
    static char gain_settings_store[MAX_GAIN_SETTINGS_SIZE];
    if (st->dev) {
        int offset = 0;
        if (SoapySDRDevice_hasGainMode(st->dev, SOAPY_SDR_RX, 0)) {
            offset += snprintf(&gain_settings_store[offset], MAX_GAIN_SETTINGS_SIZE - 1 - offset, "AGC=%d,", SoapySDRDevice_getGainMode(st->dev, SOAPY_SDR_RX, 0) ? 1 : 0);
        }
        size_t n_gains;
        char **gain_names = SoapySDRDevice_listGains(st->dev, SOAPY_SDR_RX, 0, &n_gains);
        for (int i = 0; i < (int)n_gains; i++) {
            offset += snprintf(&gain_settings_store[offset], MAX_GAIN_SETTINGS_SIZE - 1 - offset, "%s=%f,",
                               gain_names[i],  SoapySDRDevice_getGainElement(st->dev, SOAPY_SDR_RX, 0, gain_names[i]));
        }
        gain_settings_store[offset] = '\0';
        *gain_settings = gain_settings_store;
    } else {
        *gain_settings = st->gain_settings;
    }
}

int nrsc5_set_gain(nrsc5_t *st, char *gain_settings)
{
    if (strcmp(st->gain_settings, gain_settings) == 0)
        return 0;
    if (!st->stopped)
        return 1;

    if (st->dev)
    {
        int len = strlen(gain_settings);
        int offset;
        for (offset = 0; offset < len; ) {
            char gain_name[21];
            float gain_value;
            int consumed;
            int n = sscanf(&gain_settings[offset], "%20[^=]=%f%n", gain_name, &gain_value, &consumed);
            if (n != 2)
                return 1;
            if (strcmp(gain_name, "AGC") == 0 && SoapySDRDevice_hasGainMode(st->dev, SOAPY_SDR_RX, 0)) {
                SoapySDRDevice_setGainMode(st->dev, SOAPY_SDR_RX, 0, gain_value != 0);
            } else {
                SoapySDRDevice_setGainElement(st->dev, SOAPY_SDR_RX, 0, gain_name, gain_value);
            }
            offset += consumed;
            offset += strspn(&gain_settings[offset], " /,");
        }
    }

    strncpy(st->gain_settings, gain_settings, MAX_GAIN_SETTINGS_SIZE - 1);
    return 0;
}

void nrsc5_set_callback(nrsc5_t *st, nrsc5_callback_t callback, void *opaque)
{
    if (using_worker(st))
        pthread_mutex_lock(&st->worker_mutex);
    st->callback = callback;
    st->callback_opaque = opaque;
    if (using_worker(st))
        pthread_mutex_unlock(&st->worker_mutex);
}

int nrsc5_pipe_samples_cu8(nrsc5_t *st, const uint8_t *samples, unsigned int length)
{
    unsigned int sample_groups;

    while (st->leftover_u8_num > 0 && length > 0)
    {
        st->leftover_u8[st->leftover_u8_num++] = samples[0];
        samples++;
        length--;

        if (st->leftover_u8_num == 4) {
            input_push_cu8(&st->input, st->leftover_u8, 4);
            st->leftover_u8_num = 0;
            break;
        }
    }

    sample_groups = length / 4;
    input_push_cu8(&st->input, samples, sample_groups * 4);
    samples += (sample_groups * 4);
    length -= (sample_groups * 4);

    while (length > 0)
    {
        st->leftover_u8[st->leftover_u8_num++] = samples[0];
        samples++;
        length--;
    }

    return 0;
}

int nrsc5_pipe_samples_cs16(nrsc5_t *st, const int16_t *samples, unsigned int length)
{
    unsigned int sample_groups;

    if (st->leftover_s16_num == 1 && length > 0)
    {
        st->leftover_s16[st->leftover_s16_num++] = samples[0];
        samples++;
        length--;

        input_push_cs16(&st->input, st->leftover_s16, 2);
        st->leftover_s16_num = 0;
    }

    sample_groups = length / 2;
    input_push_cs16(&st->input, samples, sample_groups * 2);
    samples += (sample_groups * 2);
    length -= (sample_groups * 2);

    if (length == 1)
        st->leftover_s16[st->leftover_s16_num++] = samples[0];

    return 0;
}
