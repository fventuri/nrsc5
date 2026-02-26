/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ao/ao.h>
#include <getopt.h>
#include <math.h>
#include <nrsc5.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef __MINGW32__
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fcntl.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <termios.h>
#endif

#include "bitwriter.h"
#include "log.h"
#include "main.h"

static void help(const char *progname)
{
    fprintf(stderr, "Usage: %s [-v] [-q] [--am] [-l log-level] [-d device-serial-number] [-p ppm-error] [-g gainRF.gainIF] [-r iq-input] [-w iq-output] [-o audio-output] [-t audio-type] [-T] [-A antenna] [--dump-hdc hdc-output] [--dump-aas-files directory] frequency program\n", progname);
}

static int parse_args(state_t *st, int argc, char *argv[])
{
    static const struct option long_opts[] = {
        { "dump-aas-files", required_argument, NULL, 1 },
        { "dump-hdc", required_argument, NULL, 2 },
        { "am", no_argument, NULL, 3 },
        { 0 }
    };
    const char *version = NULL;
    char *output_name = NULL, *audio_name = NULL, *hdc_name = NULL;
    char *audio_type = "wav";
    char *endptr;
    int opt;

    st->mode = NRSC5_MODE_FM;
    st->gain = -1;
    st->bias_tee = 0;
    st->antenna = NULL;
    st->ppm_error = INT_MIN;
    log_set_level(LOG_INFO);

    while ((opt = getopt_long(argc, argv, "r:w:o:t:d:p:g:ql:vTA:", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        case 1:
            st->aas_files_path = strdup(optarg);
            break;
        case 2:
            hdc_name = optarg;
            break;
        case 3:
            st->mode = NRSC5_MODE_AM;
            break;
        case 'r':
            st->input_name = strdup(optarg);
            break;
        case 'w':
            output_name = optarg;
            break;
        case 'o':
            audio_name = optarg;
            break;
        case 't':
            if ((strcmp(optarg, "wav") != 0) && (strcmp(optarg, "raw") != 0))
            {
                log_fatal("Audio type must be either wav or raw.");
                return -1;
            }
            audio_type = optarg;
            break;
        case 'd':
            st->device_serial = strdup(optarg);
            break;
        case 'p':
            st->ppm_error = strtol(optarg, NULL, 10);
            break;
        case 'g':
            st->gain = strtof(optarg, &endptr);
            if (*endptr != 0)
            {
                log_fatal("Invalid gain.");
                return -1;
            }
            break;
        case 'q':
            log_set_quiet(1);
            break;
        case 'l':
            log_set_level(atoi(optarg));
            break;
        case 'v':
            nrsc5_get_version(&version);
            printf("nrsc5 revision %s\n", version);
            return 1;
        case 'T':
            st->bias_tee = 1;
            break;
        case 'A':
            st->antenna = strdup(optarg);
            break;
        default:
            help(argv[0]);
            return 1;
        }
    }

    if (optind + (!st->input_name + 1) != argc)
    {
        help(argv[0]);
        return 1;
    }

    if (!st->input_name)
    {
        st->freq = strtof(argv[optind++], &endptr);
        if (*endptr != 0)
        {
            log_fatal("Invalid frequency.");
            return -1;
        }

        // compatibility with previous versions
        if (st->freq < 10000.0f)
            st->freq *= 1e6f;
    }

    st->program = strtoul(argv[optind++], &endptr, 0);
    if (*endptr != 0)
    {
        log_fatal("Invalid program.");
        return -1;
    }

    if (audio_name)
        st->dev = open_ao_file(audio_name, audio_type);
    else
        st->dev = open_ao_live();

    if (st->dev == NULL)
    {
        log_fatal("Unable to open audio device.");
        return 1;
    }

    if (output_name)
    {
        if (strcmp(output_name, "-") == 0)
            st->iq_file = stdout;
        else
            st->iq_file = fopen(output_name, "wb");
        if (st->iq_file == NULL)
        {
            log_fatal("Unable to open IQ output.");
            return 1;
        }
    }

    if (hdc_name)
    {
        if (strcmp(hdc_name, "-") == 0)
            st->hdc_file = stdout;
        else
            st->hdc_file = fopen(hdc_name, "wb");
        if (st->hdc_file == NULL)
        {
            log_fatal("Unable to open HDC output.");
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pthread_mutex_t log_mutex;
    pthread_t input_thread;
    nrsc5_t *radio = NULL;
    state_t *st = calloc(1, sizeof(state_t));

    pthread_mutex_init(&log_mutex, NULL);
    log_set_lock(log_lock);
    log_set_udata(&log_mutex);

    ao_initialize();
    init_audio_buffers(st);
    if (parse_args(st, argc, argv) != 0)
        return 0;

#ifdef __MINGW32__
    SetConsoleOutputCP(CP_UTF8);
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    if (st->input_name)
    {
        FILE *fp = strcmp(st->input_name, "-") == 0 ? stdin : fopen(st->input_name, "rb");
        if (fp == NULL)
        {
            log_fatal("Open IQ file failed.");
            return 1;
        }
        if (nrsc5_open_file(&radio, fp) != 0)
        {
            log_fatal("Open IQ failed.");
            return 1;
        }
    }
    else
    {
        if (nrsc5_open(&radio, st->device_serial, st->antenna) != 0)
        {
            log_fatal("Open device failed.");
            return 1;
        }
    }
    if (nrsc5_set_bias_tee(radio, st->bias_tee) != 0)
    {
        log_fatal("Set bias-T failed.");
        return 1;
    }
    if (nrsc5_set_antenna(radio, st->antenna) != 0)
    {
        log_fatal("Set antenna failed.");
        return 1;
    }
    if (st->ppm_error != INT_MIN && nrsc5_set_freq_correction(radio, st->ppm_error) != 0)
    {
        log_fatal("Set frequency correction failed.");
        return 1;
    }
    if (nrsc5_set_frequency(radio, st->freq) != 0)
    {
        log_fatal("Set frequency failed.");
        return 1;
    }
    nrsc5_set_mode(radio, st->mode);
    if (st->gain >= 0.0f)
        nrsc5_set_gain(radio, st->gain);
    nrsc5_set_callback(radio, callback, st);
    nrsc5_start(radio);

    pthread_create(&input_thread, NULL, input_main, st);

    while (1)
    {
        audio_buffer_t *b;

        pthread_mutex_lock(&st->mutex);
        while (!st->done && (st->head == NULL || st->audio_ready < AUDIO_THRESHOLD))
            pthread_cond_wait(&st->cond, &st->mutex);

        // exit once done and no more audio buffers
        if (st->head == NULL)
        {
            pthread_mutex_unlock(&st->mutex);
            break;
        }

        // unlink from head list
        b = st->head;
        st->head = b->next;
        if (st->head == NULL)
            st->tail = NULL;
        pthread_mutex_unlock(&st->mutex);

        ao_play(st->dev, b->data, sizeof(b->data));

        pthread_mutex_lock(&st->mutex);
        // add to free list
        b->next = st->free;
        st->free = b;
        pthread_cond_signal(&st->cond);
        pthread_mutex_unlock(&st->mutex);
    }

    pthread_cancel(input_thread);
    pthread_join(input_thread, NULL);

    nrsc5_stop(radio);
    nrsc5_set_bias_tee(radio, 0);
    nrsc5_close(radio);
    cleanup(st);
    free(st);
    ao_shutdown();
    return 0;
}
