#include <assert.h>
#include <string.h>

#include "private.h"

pthread_mutex_t fftw_mutex = PTHREAD_MUTEX_INITIALIZER;

void nrsc5_get_version(const char **version)
{
    *version = GIT_COMMIT_HASH;
}

void nrsc5_service_data_type_name(unsigned int type, const char **name)
{
    switch (type)
    {
    case NRSC5_SERVICE_DATA_TYPE_NON_SPECIFIC: *name = "Non-specific"; break;
    case NRSC5_SERVICE_DATA_TYPE_NEWS: *name = "News"; break;
    case NRSC5_SERVICE_DATA_TYPE_SPORTS: *name = "Sports"; break;
    case NRSC5_SERVICE_DATA_TYPE_WEATHER: *name = "Weather"; break;
    case NRSC5_SERVICE_DATA_TYPE_EMERGENCY: *name = "Emergency"; break;
    case NRSC5_SERVICE_DATA_TYPE_TRAFFIC: *name = "Traffic"; break;
    case NRSC5_SERVICE_DATA_TYPE_IMAGE_MAPS: *name = "Image Maps"; break;
    case NRSC5_SERVICE_DATA_TYPE_TEXT: *name = "Text"; break;
    case NRSC5_SERVICE_DATA_TYPE_ADVERTISING: *name = "Advertising"; break;
    case NRSC5_SERVICE_DATA_TYPE_FINANCIAL: *name = "Financial"; break;
    case NRSC5_SERVICE_DATA_TYPE_STOCK_TICKER: *name = "Stock Ticker"; break;
    case NRSC5_SERVICE_DATA_TYPE_NAVIGATION: *name = "Navigation"; break;
    case NRSC5_SERVICE_DATA_TYPE_ELECTRONIC_PROGRAM_GUIDE: *name = "Electronic Program Guide"; break;
    case NRSC5_SERVICE_DATA_TYPE_AUDIO: *name = "Audio"; break;
    case NRSC5_SERVICE_DATA_TYPE_PRIVATE_DATA_NETWORK: *name = "Private Data Network"; break;
    case NRSC5_SERVICE_DATA_TYPE_SERVICE_MAINTENANCE: *name = "Service Maintenance"; break;
    case NRSC5_SERVICE_DATA_TYPE_HD_RADIO_SYSTEM_SERVICES: *name = "HD Radio System Services"; break;
    case NRSC5_SERVICE_DATA_TYPE_AUDIO_RELATED_DATA: *name = "Audio-Related Objects"; break;
    case NRSC5_SERVICE_DATA_TYPE_RESERVED_FOR_SPECIAL_TESTS: *name = "Reserved for Special Tests"; break;
    default: *name = "Unknown"; break;
    }
}

void nrsc5_program_type_name(unsigned int type, const char **name)
{
    switch (type)
    {
    case NRSC5_PROGRAM_TYPE_UNDEFINED: *name = "None"; break;
    case NRSC5_PROGRAM_TYPE_NEWS: *name = "News"; break;
    case NRSC5_PROGRAM_TYPE_INFORMATION: *name = "Information"; break;
    case NRSC5_PROGRAM_TYPE_SPORTS: *name = "Sports"; break;
    case NRSC5_PROGRAM_TYPE_TALK: *name = "Talk"; break;
    case NRSC5_PROGRAM_TYPE_ROCK: *name = "Rock"; break;
    case NRSC5_PROGRAM_TYPE_CLASSIC_ROCK: *name = "Classic Rock"; break;
    case NRSC5_PROGRAM_TYPE_ADULT_HITS: *name = "Adult Hits"; break;
    case NRSC5_PROGRAM_TYPE_SOFT_ROCK: *name = "Soft Rock"; break;
    case NRSC5_PROGRAM_TYPE_TOP_40: *name = "Top 40"; break;
    case NRSC5_PROGRAM_TYPE_COUNTRY: *name = "Country"; break;
    case NRSC5_PROGRAM_TYPE_OLDIES: *name = "Oldies"; break;
    case NRSC5_PROGRAM_TYPE_SOFT: *name = "Soft"; break;
    case NRSC5_PROGRAM_TYPE_NOSTALGIA: *name = "Nostalgia"; break;
    case NRSC5_PROGRAM_TYPE_JAZZ: *name = "Jazz"; break;
    case NRSC5_PROGRAM_TYPE_CLASSICAL: *name = "Classical"; break;
    case NRSC5_PROGRAM_TYPE_RHYTHM_AND_BLUES: *name = "Rhythm and Blues"; break;
    case NRSC5_PROGRAM_TYPE_SOFT_RHYTHM_AND_BLUES: *name = "Soft Rhythm and Blues"; break;
    case NRSC5_PROGRAM_TYPE_FOREIGN_LANGUAGE: *name = "Foreign Language"; break;
    case NRSC5_PROGRAM_TYPE_RELIGIOUS_MUSIC: *name = "Religious Music"; break;
    case NRSC5_PROGRAM_TYPE_RELIGIOUS_TALK: *name = "Religious Talk"; break;
    case NRSC5_PROGRAM_TYPE_PERSONALITY: *name = "Personality"; break;
    case NRSC5_PROGRAM_TYPE_PUBLIC: *name = "Public"; break;
    case NRSC5_PROGRAM_TYPE_COLLEGE: *name = "College"; break;
    case NRSC5_PROGRAM_TYPE_SPANISH_TALK: *name = "Spanish Talk"; break;
    case NRSC5_PROGRAM_TYPE_SPANISH_MUSIC: *name = "Spanish Music"; break;
    case NRSC5_PROGRAM_TYPE_HIP_HOP: *name = "Hip-Hop"; break;
    case NRSC5_PROGRAM_TYPE_WEATHER: *name = "Weather"; break;
    case NRSC5_PROGRAM_TYPE_EMERGENCY_TEST: *name = "Emergency Test"; break;
    case NRSC5_PROGRAM_TYPE_EMERGENCY: *name = "Emergency"; break;
    case NRSC5_PROGRAM_TYPE_TRAFFIC: *name = "Traffic"; break;
    case NRSC5_PROGRAM_TYPE_SPECIAL_READING_SERVICES: *name = "Special Reading Services"; break;
    default: *name = "Unknown"; break;
    }
}

void nrsc5_alert_category_name(unsigned int type, const char **name)
{
    switch (type)
    {
    case NRSC5_ALERT_CATEGORY_NON_SPECIFIC: *name = "Non-specific"; break;
    case NRSC5_ALERT_CATEGORY_GEOPHYSICAL: *name = "Geophysical"; break;
    case NRSC5_ALERT_CATEGORY_WEATHER: *name = "Weather"; break;
    case NRSC5_ALERT_CATEGORY_SAFETY: *name = "Safety"; break;
    case NRSC5_ALERT_CATEGORY_SECURITY: *name = "Security"; break;
    case NRSC5_ALERT_CATEGORY_RESCUE: *name = "Rescue"; break;
    case NRSC5_ALERT_CATEGORY_FIRE: *name = "Fire"; break;
    case NRSC5_ALERT_CATEGORY_HEALTH: *name = "Health"; break;
    case NRSC5_ALERT_CATEGORY_ENVIRONMENTAL: *name = "Environmental"; break;
    case NRSC5_ALERT_CATEGORY_TRANSPORTATION: *name = "Transportation"; break;
    case NRSC5_ALERT_CATEGORY_UTILITIES: *name = "Utilities"; break;
    case NRSC5_ALERT_CATEGORY_HAZMAT: *name = "Hazmat"; break;
    case NRSC5_ALERT_CATEGORY_TEST: *name = "Test"; break;
    default: *name = "Unknown"; break;
    }
}

void nrsc5_report(nrsc5_t *st, const nrsc5_event_t *evt)
{
    if (st->callback)
        st->callback(evt, st->callback_opaque);
}

void nrsc5_report_lost_device(nrsc5_t *st)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOST_DEVICE;
    nrsc5_report(st, &evt);
}

void nrsc5_report_agc(nrsc5_t *st, float gain_db, float peak_dbfs, int is_final)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_AGC;
    evt.agc.gain_db = gain_db;
    evt.agc.peak_dbfs = peak_dbfs;
    evt.agc.is_final = is_final;
    nrsc5_report(st, &evt);
}

void nrsc5_report_iq(nrsc5_t *st, const void *data, size_t count)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_IQ;
    evt.iq.data = data;
    evt.iq.count = count;
    nrsc5_report(st, &evt);
}

void nrsc5_report_sync(nrsc5_t *st, float freq_offset, int psmi)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SYNC;
    evt.sync.freq_offset = freq_offset;
    evt.sync.psmi = psmi;
    nrsc5_report(st, &evt);
}

void nrsc5_report_lost_sync(nrsc5_t *st)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOST_SYNC;
    nrsc5_report(st, &evt);
}

void nrsc5_report_hdc(nrsc5_t *st, unsigned int program, const packet_t* pkt)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_HDC;
    evt.hdc.program = program;
    evt.hdc.data = NULL;
    evt.hdc.count = 0;
    evt.hdc.flags = NRSC5_PKT_FLAGS_NONE;

    if (pkt->shape == PACKET_FULL)
    {
        evt.hdc.data = pkt->data;
        evt.hdc.count = pkt->size;
    }
    if (pkt->flags & PACKET_FLAG_CRC_ERROR)
        evt.hdc.flags |= NRSC5_PKT_FLAGS_CRC_ERROR;

    nrsc5_report(st, &evt);
}

void nrsc5_report_audio(nrsc5_t *st, unsigned int program, const int16_t *data, size_t count)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_AUDIO;
    evt.audio.program = program;
    evt.audio.data = data;
    evt.audio.count = count;
    nrsc5_report(st, &evt);
}

void nrsc5_report_mer(nrsc5_t *st, float lower, float upper)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_MER;
    evt.mer.lower = lower;
    evt.mer.upper = upper;
    nrsc5_report(st, &evt);
}

void nrsc5_report_ber(nrsc5_t *st, float cber)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_BER;
    evt.ber.cber = cber;
    nrsc5_report(st, &evt);
}

void nrsc5_report_stream(nrsc5_t *st, uint16_t seq, unsigned int size, const uint8_t *data,
                         nrsc5_sig_service_t *service, nrsc5_sig_component_t *component)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STREAM;
    evt.stream.port = component->data.port;
    evt.stream.seq = seq;
    evt.stream.size = size;
    evt.stream.mime = component->data.mime;
    evt.stream.data = data;
    evt.stream.service = service;
    evt.stream.component = component;
    nrsc5_report(st, &evt);
}

void nrsc5_report_packet(nrsc5_t *st, uint16_t seq, unsigned int size, const uint8_t *data,
                         nrsc5_sig_service_t *service, nrsc5_sig_component_t *component)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_PACKET;
    evt.packet.port = component->data.port;
    evt.packet.seq = seq;
    evt.packet.size = size;
    evt.packet.mime = component->data.mime;
    evt.packet.data = data;
    evt.packet.service = service;
    evt.packet.component = component;
    nrsc5_report(st, &evt);
}

void nrsc5_report_lot(nrsc5_t *st, int event, unsigned int lot, unsigned int size, uint32_t mime,
                      const char *name, const uint8_t *data, struct tm *expiry_utc,
                      nrsc5_sig_service_t *service, nrsc5_sig_component_t *component)
{
    nrsc5_event_t evt;

    evt.event = event;
    evt.lot.port = component->data.port;
    evt.lot.lot = lot;
    evt.lot.size = size;
    evt.lot.mime = mime;
    evt.lot.name = name;
    evt.lot.data = data;
    evt.lot.expiry_utc = expiry_utc;
    evt.lot.service = service;
    evt.lot.component = component;
    nrsc5_report(st, &evt);
}

void nrsc5_report_lot_fragment(nrsc5_t *st, unsigned int lot, unsigned int seq, unsigned int repeat, int is_duplicate,
                               unsigned int size, unsigned int bytes_so_far, const uint8_t *data,
                               nrsc5_sig_service_t *service, nrsc5_sig_component_t *component)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOT_FRAGMENT;
    evt.lot_fragment.lot = lot;
    evt.lot_fragment.seq = seq;
    evt.lot_fragment.repeat = repeat;
    evt.lot_fragment.is_duplicate = is_duplicate;
    evt.lot_fragment.size = size;
    evt.lot_fragment.bytes_so_far = bytes_so_far;
    evt.lot_fragment.data = data;
    evt.lot_fragment.service = service;
    evt.lot_fragment.component = component;
    nrsc5_report(st, &evt);
}

static uint8_t convert_sig_component_type(uint8_t type)
{
    switch (type)
    {
    case SIG_COMPONENT_AUDIO: return NRSC5_SIG_COMPONENT_AUDIO;
    case SIG_COMPONENT_DATA: return NRSC5_SIG_COMPONENT_DATA;
    default:
        assert(0 && "Invalid component type");
        return 0;
    }
}

static uint8_t convert_sig_service_type(uint8_t type)
{
    switch (type)
    {
    case SIG_SERVICE_AUDIO: return NRSC5_SIG_SERVICE_AUDIO;
    case SIG_SERVICE_DATA: return NRSC5_SIG_SERVICE_DATA;
    default:
        assert(0 && "Invalid service type");
        return 0;
    }
}

void nrsc5_report_audio_service(nrsc5_t *st, unsigned int program, unsigned int access, unsigned int type, 
                                unsigned int codec_mode, unsigned int blend_control, int digital_audio_gain,
                                unsigned int common_delay, unsigned int latency)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_AUDIO_SERVICE;
    evt.audio_service.access = access;
    evt.audio_service.program = program;
    evt.audio_service.type = type;
    evt.audio_service.codec_mode = codec_mode;
    evt.audio_service.blend_control = blend_control;
    evt.audio_service.digital_audio_gain = digital_audio_gain;
    evt.audio_service.common_delay = common_delay;
    evt.audio_service.latency = latency;
    nrsc5_report(st, &evt);
}

void nrsc5_report_sig(nrsc5_t *st, sig_service_t *services)
{
    nrsc5_sig_service_t *service = NULL;
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SIG;

    // convert internal structures to public structures
    for (unsigned int i = 0; i < MAX_SIG_SERVICES; i++)
    {
        nrsc5_sig_component_t *component = NULL;
        nrsc5_sig_service_t *prev = service;

        if (services[i].type == SIG_SERVICE_NONE)
            break;

        service = calloc(1, sizeof(nrsc5_sig_service_t));
        service->type = convert_sig_service_type(services[i].type);
        service->number = services[i].number;
        service->name = services[i].name;

        if (prev == NULL)
            evt.sig.services = service;
        else
            prev->next = service;

        for (unsigned int j = 0; j < MAX_SIG_COMPONENTS; j++)
        {
            nrsc5_sig_component_t *prevc = component;
            sig_component_t *internal = &services[i].component[j];

            if (internal->type == SIG_COMPONENT_NONE)
                continue;

            component = calloc(1, sizeof(nrsc5_sig_component_t));
            component->type = convert_sig_component_type(internal->type);
            component->id = internal->id;

            if (internal->type == SIG_COMPONENT_AUDIO)
            {
                component->audio.port = internal->audio.port;
                component->audio.type = internal->audio.type;
                component->audio.mime = internal->audio.mime;
                service->audio_component = component;
            }
            else if (internal->type == SIG_COMPONENT_DATA)
            {
                component->data.port = internal->data.port;
                component->data.service_data_type = internal->data.service_data_type;
                component->data.type = internal->data.type;
                component->data.mime = internal->data.mime;
            }

            // cache the service & component records for use in API events
            internal->service_ext = service;
            internal->component_ext = component;

            if (prevc == NULL)
                service->components = component;
            else
                prevc->next = component;
        }
    }

    nrsc5_report(st, &evt);
    st->sig_table = evt.sig.services;
}

void nrsc5_clear_sig(nrsc5_t *st)
{
    nrsc5_sig_service_t *service = NULL;

    // free the data structures
    for (service = st->sig_table; service != NULL; )
    {
        void *p;
        nrsc5_sig_component_t *component;

        for (component = service->components; component != NULL; )
        {
            p = component;
            component = component->next;
            free(p);
        }

        p = service;
        service = service->next;
        free(p);
    }
    st->sig_table = NULL;
}

void nrsc5_report_sis(nrsc5_t *st, const char *country_code, int fcc_facility_id, const char *name,
                      const char *slogan, const char *message, const char *alert, const uint8_t *cnt, int cnt_length,
                      int category1, int category2, int location_format, int num_locations, const int *locations,
                      float latitude, float longitude, int altitude, nrsc5_sis_asd_t *audio_services,
                      nrsc5_sis_dsd_t *data_services)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SIS;
    evt.sis.country_code = country_code;
    evt.sis.fcc_facility_id = fcc_facility_id;
    evt.sis.name = name;
    evt.sis.slogan = slogan;
    evt.sis.message = message;
    evt.sis.alert = alert;
    evt.sis.alert_cnt = cnt;
    evt.sis.alert_cnt_length = cnt_length;
    evt.sis.alert_category1 = category1;
    evt.sis.alert_category2 = category2;
    evt.sis.alert_location_format = location_format;
    evt.sis.alert_num_locations = num_locations;
    evt.sis.alert_locations = locations;
    evt.sis.latitude = latitude;
    evt.sis.longitude = longitude;
    evt.sis.altitude = altitude;
    evt.sis.audio_services = audio_services;
    evt.sis.data_services = data_services;

    nrsc5_report(st, &evt);
}

void nrsc5_report_station_id(nrsc5_t *st, const char *country_code, int fcc_facility_id)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STATION_ID;
    evt.station_id.country_code = country_code;
    evt.station_id.fcc_facility_id = fcc_facility_id;

    nrsc5_report(st, &evt);
}

void nrsc5_report_station_name(nrsc5_t *st, const char *name)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STATION_NAME;
    evt.station_name.name = name;

    nrsc5_report(st, &evt);
}

void nrsc5_report_station_slogan(nrsc5_t *st, const char *slogan)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STATION_SLOGAN;
    evt.station_slogan.slogan = slogan;

    nrsc5_report(st, &evt);
}

void nrsc5_report_station_message(nrsc5_t *st, const char *message)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STATION_MESSAGE;
    evt.station_message.message = message;

    nrsc5_report(st, &evt);
}

void nrsc5_report_station_location(nrsc5_t *st, float latitude, float longitude, int altitude)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_STATION_LOCATION;
    evt.station_location.latitude = latitude;
    evt.station_location.longitude = longitude;
    evt.station_location.altitude = altitude;

    nrsc5_report(st, &evt);
}

void nrsc5_report_asd(nrsc5_t *st, unsigned int program, unsigned int access, unsigned int type, unsigned int sound_exp)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_AUDIO_SERVICE_DESCRIPTOR;
    evt.asd.program = program;
    evt.asd.access = access;
    evt.asd.type = type;
    evt.asd.sound_exp = sound_exp;

    nrsc5_report(st, &evt);
}

void nrsc5_report_dsd(nrsc5_t *st, unsigned int access, unsigned int type, uint32_t mime_type)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_DATA_SERVICE_DESCRIPTOR;
    evt.dsd.access = access;
    evt.dsd.type = type;
    evt.dsd.mime_type = mime_type;

    nrsc5_report(st, &evt);
}

void nrsc5_report_emergency_alert(nrsc5_t *st, const char *message, const uint8_t *control_data,
                                  int control_data_length, int category1, int category2, int location_format,
                                  int num_locations, const int *locations)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_EMERGENCY_ALERT;
    evt.emergency_alert.message = message;
    evt.emergency_alert.control_data = control_data;
    evt.emergency_alert.control_data_length = control_data_length;
    evt.emergency_alert.category1 = category1;
    evt.emergency_alert.category2 = category2;
    evt.emergency_alert.location_format = location_format;
    evt.emergency_alert.num_locations = num_locations;
    evt.emergency_alert.locations = locations;

    nrsc5_report(st, &evt);
}

void nrsc5_report_here_image(nrsc5_t *st, int image_type, int seq, int n1, int n2, unsigned int timestamp,
                             float latitude1, float longitude1, float latitude2, float longitude2,
                             const char *name, unsigned int size, const uint8_t *data)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_HERE_IMAGE;
    evt.here_image.image_type = image_type;
    evt.here_image.seq = seq;
    evt.here_image.n1 = n1;
    evt.here_image.n2 = n2;
#if defined(WIN32) || defined(_WIN32)
    __time64_t ts = (__time64_t) timestamp;
    evt.here_image.time_utc = _gmtime64(&ts);
#else
    time_t ts = (time_t) timestamp;
    evt.here_image.time_utc = gmtime(&ts);
#endif
    evt.here_image.latitude1 = latitude1;
    evt.here_image.longitude1 = longitude1;
    evt.here_image.latitude2 = latitude2;
    evt.here_image.longitude2 = longitude2;
    evt.here_image.name = name;
    evt.here_image.size = size;
    evt.here_image.data = data;

    nrsc5_report(st, &evt);
}
