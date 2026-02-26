#include <assert.h>
#include <string.h>

#include "private.h"

// SDRplay RX and event callbacks
static void stream_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
                            unsigned int numSamples, unsigned int reset, void *cbContext);
static void event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
                           sdrplay_api_EventParamsT *params, void *cbContext);


static int using_worker(nrsc5_t *st)
{
    return st->iq_file != NULL;
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

            if (st->iq_file)
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
    st->gain = 0;
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

int nrsc5_open(nrsc5_t **result, char *device_serial, char *antenna)
{
    sdrplay_api_ErrT err;
    nrsc5_t *st = nrsc5_alloc();

    if ((err = sdrplay_api_Open()) != sdrplay_api_Success) {
        log_error("sdrplay_api_Open() error: %d", err);
        goto error_init;
    }

    unsigned int n_devs = 0;

    sdrplay_api_LockDeviceApi();
    sdrplay_api_DeviceT devs[SDRPLAY_MAX_DEVICES];
    sdrplay_api_GetDevices(devs, &n_devs, SDRPLAY_MAX_DEVICES);

    unsigned int dev_idx = 0;
    if (device_serial) {
        int device_serial_length = strlen(device_serial);
        if (device_serial_length == 1) {
            sscanf(device_serial, "%u", &dev_idx);
        } else if (device_serial_length > 1) {
            for (dev_idx = 0; dev_idx < n_devs; ++dev_idx) {
                if (devs[dev_idx].valid && strcmp(devs[dev_idx].SerNo, device_serial) == 0)
                    break;
            }
        }
    }
    if (dev_idx >= n_devs) {
        log_error("RSP not found (RSPs: %d)", n_devs);
        goto error_release_api_lock;
    }
    st->dev = devs[dev_idx];
    if (st->dev.hwVer == SDRPLAY_RSPduo_ID) {
        if (st->dev.rspDuoMode & sdrplay_api_RspDuoMode_Single_Tuner) {
            st->dev.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
            if (antenna && (strcmp(antenna, "Tuner 2") == 0 || strncmp(antenna, "Tuner 2 ", 8) == 0)) {
                st->dev.tuner = sdrplay_api_Tuner_B;
            } else {
                st->dev.tuner = sdrplay_api_Tuner_A;
            } 
        } else {
            log_error("RSPduo not in Single Tuner mode");
            goto error_release_api_lock;
        }
    }

    if ((err = sdrplay_api_SelectDevice(&st->dev)) != sdrplay_api_Success) {
        log_error("sdrplay_api_SelectDevice() error: %d", err);
        goto error_release_api_lock;
    }
    sdrplay_api_UnlockDeviceApi();
    if ((err = sdrplay_api_GetDeviceParams(st->dev.dev, &st->dev_params)) != sdrplay_api_Success) {
        log_error("sdrplay_api_GetDeviceParams() error: %d", err);
        goto error;
    }
    st->ch_params = st->dev.tuner == sdrplay_api_Tuner_B ? st->dev_params->rxChannelB : st->dev_params->rxChannelA;

    /* default settings:
     *   - rf: 200MHz
     *   - fs: 2MHz
     *   - decimation: off
     *   - IF: 0kHz (zero IF)
     *   - bw: 200kHz
     *   - attenuation: 50dB
     *   - LNA state: 0
     *   - AGC: 50Hz
     *   - DC correction: on
     *   - IQ balance: on
    */

    int decimation = 4;
    st->ch_params->ctrlParams.decimation.enable = 1;
    st->ch_params->ctrlParams.decimation.decimationFactor = decimation;
#if 0
    st->ch_params->ctrlParams.decimation.wideBandSignal = 1;
#endif
    st->dev_params->devParams->fsFreq.fsHz = NRSC5_SAMPLE_RATE_CS16_FM * decimation;
    /* increase bandwidth since NRSC5 requires about 400kHz */
    st->ch_params->tunerParams.bwType = sdrplay_api_BW_0_600;

    nrsc5_init(st);

    *result = st;
    return 0;

error:
    log_error("nrsc5_open error: %d", err);
    sdrplay_api_LockDeviceApi();
    sdrplay_api_ReleaseDevice(&st->dev);
error_release_api_lock:
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
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

    if (st->dev.SerNo[0]) {
        sdrplay_api_LockDeviceApi();
        sdrplay_api_ReleaseDevice(&st->dev);
        sdrplay_api_UnlockDeviceApi();
        sdrplay_api_Close();
    }
    if (st->iq_file)
        fclose(st->iq_file);

    input_free(&st->input);
    output_free(&st->output);
    free(st);
}

void nrsc5_start(nrsc5_t *st)
{
    sdrplay_api_CallbackFnsT callbacks;
    callbacks.StreamACbFn = stream_callback;
    callbacks.StreamBCbFn = 0;
    callbacks.EventCbFn = event_callback;

/* fv - debug info */
if (st->dev.hwVer == SDRPLAY_RSP2_ID) {
    fprintf(stderr, "antenna: %d\n", st->ch_params->rsp2TunerParams.antennaSel);
    fprintf(stderr, "AM port: %d\n", st->ch_params->rsp2TunerParams.amPortSel);
} else if (st->dev.hwVer == SDRPLAY_RSPduo_ID) {
    fprintf(stderr, "tuner: %d\n", st->dev.tuner);
    fprintf(stderr, "AM port: %d\n", st->ch_params->rspDuoTunerParams.tuner1AmPortSel);
} else if (st->dev.hwVer == SDRPLAY_RSPdx_ID || st->dev.hwVer == SDRPLAY_RSPdxR2_ID) {
    fprintf(stderr, "antenna: %d\n", st->dev_params->devParams->rspDxParams.antennaSel);
}
fprintf(stderr, "LNA state: %d\n", st->ch_params->tunerParams.gain.LNAstate);
fprintf(stderr, "IF gain reduction: %d\n", st->ch_params->tunerParams.gain.gRdB);
fprintf(stderr, "IF AGC: %d\n", st->ch_params->ctrlParams.agc.enable);
/* fv - end debug info */

    sdrplay_api_ErrT err;
    if ((err = sdrplay_api_Init(st->dev.dev, &callbacks, (void *)st)) != sdrplay_api_Success) {
        log_error("sdrplay_api_Init failed");
        return;
    }
    st->stopped = 0;

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
    sdrplay_api_ErrT err;
    if ((err = sdrplay_api_Uninit(st->dev.dev)) != sdrplay_api_Success) {
        log_error("sdrplay_api_Uninit failed");
        return;
    }
    st->stopped = 1;

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
    if (st->ch_params)
    {
        if (st->dev.hwVer == SDRPLAY_RSP1A_ID || st->dev.hwVer == SDRPLAY_RSP1B_ID) {
            st->ch_params->rsp1aTunerParams.biasTEnable = on;
        } else if (st->dev.hwVer == SDRPLAY_RSP2_ID) {
            st->ch_params->rsp2TunerParams.biasTEnable = on;
        } else if (st->dev.hwVer == SDRPLAY_RSPduo_ID) {
            st->ch_params->rspDuoTunerParams.biasTEnable = on;
        }
    }

    if (st->dev_params)
    {
        if (st->dev.hwVer == SDRPLAY_RSPdx_ID || st->dev.hwVer == SDRPLAY_RSPdxR2_ID) {
            st->dev_params->devParams->rspDxParams.biasTEnable = on;
        }
    }
    return 0;
}

int nrsc5_set_antenna(nrsc5_t *st, char *antenna)
{
    if (!antenna)
        return 0;

    if (st->ch_params)
    {
        if (st->dev.hwVer == SDRPLAY_RSP2_ID) {
            if (strcmp(antenna, "Antenna A") == 0) {
                st->ch_params->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_A;
                st->ch_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_2;
            } else if (strcmp(antenna, "Antenna B") == 0) {
                st->ch_params->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_B;
                st->ch_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_2;
            } else if (strcmp(antenna, "Hi-Z") == 0) {
                st->ch_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_1;
            }
        } else if (st->dev.hwVer == SDRPLAY_RSPduo_ID) {
            if (strcmp(antenna, "Tuner 1 50 ohm") == 0) {
                st->dev.tuner = sdrplay_api_Tuner_A;
                st->ch_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_2;
            } else if (strcmp(antenna, "Tuner 2 50 ohm") == 0) {
                st->dev.tuner = sdrplay_api_Tuner_B;
                st->ch_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_2;
            } else if (strcmp(antenna, "Tuner 1 Hi-Z") == 0) {
                st->dev.tuner = sdrplay_api_Tuner_A;
                st->ch_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_1;
            }
        }
    }

    if (st->dev_params)
    {
        if (st->dev.hwVer == SDRPLAY_RSPdx_ID || st->dev.hwVer == SDRPLAY_RSPdxR2_ID) {
            if (strcmp(antenna, "Antenna A") == 0) {
                st->dev_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_A;
            } else if (strcmp(antenna, "Antenna B") == 0) {
                st->dev_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_B;
            } else if (strcmp(antenna, "Antenna C") == 0) {
                st->dev_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_C;
            }
        }
    }
    return 0;
}

int nrsc5_set_freq_correction(nrsc5_t *st, int ppm_error)
{
    if (st->dev_params)
    {
        st->dev_params->devParams->ppm = ppm_error;
    }
    return 0;
}

void nrsc5_get_frequency(nrsc5_t *st, float *freq)
{
    if (st->ch_params)
        *freq = st->ch_params->tunerParams.rfFreq.rfHz;
    else
        *freq = st->freq;
}

int nrsc5_set_frequency(nrsc5_t *st, float freq)
{
    if (st->freq == freq)
        return 0;
    if (!st->stopped)
        return 1;

    if (st->ch_params)
        st->ch_params->tunerParams.rfFreq.rfHz = freq;

    input_reset(&st->input);
    output_reset(&st->output);

    st->freq = freq;
    return 0;
}

void nrsc5_get_gain(nrsc5_t *st, float *gain)
{
    if (st->ch_params) {
        float rfgr = st->ch_params->tunerParams.gain.LNAstate;
        float ifgr = st->ch_params->ctrlParams.agc.enable == sdrplay_api_AGC_DISABLE ? st->ch_params->tunerParams.gain.gRdB : 0;
        *gain = rfgr + ifgr / 100.0f;
    } else {
        *gain = st->gain;
    }
}

int nrsc5_set_gain(nrsc5_t *st, float gain)
{
    if (st->gain == gain)
        return 0;
    if (!st->stopped)
        return 1;

    if (st->ch_params) {
        int rfgr = (int) gain;
        int ifgr = (int) ((gain - rfgr) * 100.0f + 0.00001);
        st->ch_params->tunerParams.gain.LNAstate = (unsigned char) rfgr;
        if (ifgr == 0) {
            st->ch_params->ctrlParams.agc.setPoint_dBfs = -60;
            st->ch_params->ctrlParams.agc.enable = sdrplay_api_AGC_50HZ;
        } else {
            st->ch_params->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
            st->ch_params->tunerParams.gain.gRdB = ifgr;
        }
    }

    st->gain = gain;
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

// SDRplay RX and event callbacks
static void stream_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
                            unsigned int numSamples, unsigned int reset, void *cbContext)
{
    (void)params; // UNUSED
    (void)reset; // UNUSED

    nrsc5_t *st = (nrsc5_t *) cbContext;

    unsigned int num_loops = numSamples / 4;
    for (unsigned int i = 0; i < num_loops; i++) {
        st->samples_buf[8*i+0] = xi[4*i];
        st->samples_buf[8*i+2] = xi[4*i+1];
        st->samples_buf[8*i+4] = xi[4*i+2];
        st->samples_buf[8*i+6] = xi[4*i+3];
        st->samples_buf[8*i+1] = xq[4*i];
        st->samples_buf[8*i+3] = xq[4*i+1];
        st->samples_buf[8*i+5] = xq[4*i+2];
        st->samples_buf[8*i+7] = xq[4*i+3];
    }
    switch (numSamples % 4) {
        case 0:
            break;
        case 1:
            st->samples_buf[numSamples*2-2] = xi[numSamples-1];
            st->samples_buf[numSamples*2-1] = xq[numSamples-1];
            break;
        case 2:
            st->samples_buf[numSamples*2-4] = xi[numSamples-2];
            st->samples_buf[numSamples*2-2] = xi[numSamples-1];
            st->samples_buf[numSamples*2-3] = xq[numSamples-2];
            st->samples_buf[numSamples*2-1] = xq[numSamples-1];
            break;
        case 3:
            st->samples_buf[numSamples*2-6] = xi[numSamples-3];
            st->samples_buf[numSamples*2-4] = xi[numSamples-2];
            st->samples_buf[numSamples*2-2] = xi[numSamples-1];
            st->samples_buf[numSamples*2-5] = xq[numSamples-3];
            st->samples_buf[numSamples*2-3] = xq[numSamples-2];
            st->samples_buf[numSamples*2-1] = xq[numSamples-1];
            break;
    }
    input_push_cs16(&st->input, st->samples_buf, numSamples*2);
    return;
}

static void event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
                           sdrplay_api_EventParamsT *params, void *cbContext)
{
    (void)tuner; // UNUSED

    nrsc5_t *st = (nrsc5_t *) cbContext;

    /* display warning for overloads */
    switch (eventId) {
    case sdrplay_api_PowerOverloadChange:
        /* send ack back for overload events */
        switch (params->powerOverloadParams.powerOverloadChangeType) {
        case sdrplay_api_Overload_Detected:
            log_warn("overload detected - please increase gain reduction");
            break;
        case sdrplay_api_Overload_Corrected:
            log_warn("overload corrected");
            break;
        }
        sdrplay_api_Update(st->dev.dev, st->dev.tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck, sdrplay_api_Update_Ext1_None);
        break;
    /* these case are not handled (yet) */
    case sdrplay_api_GainChange:
    case sdrplay_api_DeviceRemoved:
    case sdrplay_api_RspDuoModeChange:
    case sdrplay_api_DeviceFailure:
        break;
    }
    return;
}
