/*  Copyright (c) 2003-2012 Xfce Development Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <stdarg.h>

#include "weather-debug.h"

#define INVALID_TIME "INVALID"

#define YESNO(bool) ((bool) ? "yes" : "no")


static void
weather_dummy_log_handler(const gchar *log_domain,
                          const GLogLevelFlags log_level,
                          const gchar *message,
                          const gpointer unused_data)
{
    /* Swallow all messages */
}


void
weather_debug_init(const gchar *log_domain,
                   const gboolean debug_mode)
{
    /*
     * GLIB >= 2.32 only shows debug messages if G_MESSAGES_DEBUG contains the
     * log domain or "all", earlier GLIB versions always show debugging output
     */
#if GLIB_CHECK_VERSION(2,32,0)
    const gchar *debug_env;
    gchar *debug_env_new_array[] = { NULL, NULL, NULL, NULL };
    gchar *debug_env_new;
    guint i = 0, j = 0;

    if (debug_mode) {
        debug_env = g_getenv("G_MESSAGES_DEBUG");

        if (log_domain == NULL)
            debug_env_new_array[i++] = g_strdup("all");
        else {
            if (debug_env != NULL)
                debug_env_new_array[i++] = g_strdup(debug_env);
            if (debug_env == NULL || strstr(debug_env, log_domain) == NULL)
                debug_env_new_array[i++] = g_strdup(log_domain);
            if (debug_env == NULL || strstr(debug_env, G_LOG_DOMAIN) == NULL)
                debug_env_new_array[i++] = g_strdup(G_LOG_DOMAIN);
        }
        debug_env_new = g_strjoinv(" ", debug_env_new_array);
        g_setenv("G_MESSAGES_DEBUG", debug_env_new, TRUE);
        g_free(debug_env_new);

        while (j < i)
            g_free(debug_env_new_array[j++]);
    }
#else
    if (!debug_mode) {
        g_log_set_handler(log_domain, G_LOG_LEVEL_DEBUG,
                          weather_dummy_log_handler, NULL);
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                          weather_dummy_log_handler, NULL);
    }
#endif
}


void
weather_debug_real(const gchar *log_domain,
                   const gchar *file,
                   const gchar *func,
                   const gint line,
                   const gchar *format,
                   ...)
{
    va_list args;
    gchar *prefixed_format;

    va_start(args, format);
    prefixed_format = g_strdup_printf("[%s:%d %s]: %s",
                                      file, line, func, format);
    g_logv(log_domain, G_LOG_LEVEL_DEBUG, prefixed_format, args);
    va_end(args);
    g_free(prefixed_format);
}


gchar *
weather_debug_strftime_t(const time_t t)
{
    struct tm tm;
    gchar *res;
    gchar str[20];
    size_t size;

    tm = *localtime(&t);
    size = strftime(str, 20, "%Y-%m-%d %H:%M:%S", &tm);
    return (size ? g_strdup(str) : g_strdup(INVALID_TIME));
}


gchar *
weather_dump_geolocation(const xml_geolocation *geo)
{
    gchar *out;

    if (!geo)
        return g_strdup("No geolocation data.");

    out = g_strdup_printf("Geolocation data:\n"
                          "  --------------------------------------------\n"
                          "  city: %s\n"
                          "  country name: %s\n"
                          "  country code: %s\n"
                          "  region name: %s\n"
                          "  latitude: %s\n"
                          "  longitude: %s\n"
                          "  --------------------------------------------",
                          geo->city,
                          geo->country_name,
                          geo->country_code,
                          geo->region_name,
                          geo->latitude,
                          geo->longitude);
    return out;
}


gchar *
weather_dump_place(const xml_place *place)
{
    gchar *out;

    if (!place)
        return g_strdup("No place data.");

    out = g_strdup_printf("Place data:\n"
                          "  --------------------------------------------\n"
                          "  display_name: %s\n"
                          "  latitude: %s\n"
                          "  longitude: %s\n"
                          "  --------------------------------------------",
                          place->display_name,
                          place->lat,
                          place->lon);
    return out;
}


gchar *
weather_dump_icon_theme(const icon_theme *theme)
{
    gchar *out;

    if (!theme)
        return g_strdup("No icon theme data.");

    out = g_strdup_printf("Icon theme data:\n"
                          "  --------------------------------------------\n"
                          "  Dir: %s\n"
                          "  Name: %s\n"
                          "  Author: %s\n"
                          "  Description: %s\n"
                          "  License: %s\n"
                          "  --------------------------------------------",
                          theme->dir,
                          theme->name,
                          theme->author,
                          theme->description,
                          theme->license);
    return out;
}


gchar *
weather_dump_astrodata(const xml_astro *astro)
{
    gchar *out, *sunrise, *sunset, *moonrise, *moonset;

    sunrise = weather_debug_strftime_t(astro->sunrise);
    sunset = weather_debug_strftime_t(astro->sunset);
    moonrise = weather_debug_strftime_t(astro->moonrise);
    moonset = weather_debug_strftime_t(astro->moonset);

    if (!astro)
        return g_strdup("No astronomical data.");

    out = g_strdup_printf("Astronomical data:\n"
                          "  --------------------------------------------\n"
                          "  sunrise: %s\n"
                          "  sunset: %s\n"
                          "  sun never rises: %s\n"
                          "  sun never sets: %s\n"
                          "  --------------------------------------------\n"
                          "  moonrise: %s\n"
                          "  moonset: %s\n"
                          "  moon never rises: %s\n"
                          "  moon never sets: %s\n"
                          "  moon phase: %s\n"
                          "  --------------------------------------------",
                          sunrise,
                          sunset,
                          YESNO(astro->sun_never_rises),
                          YESNO(astro->sun_never_sets),
                          moonrise,
                          moonset,
                          YESNO(astro->moon_never_rises),
                          YESNO(astro->moon_never_sets),
                          astro->moon_phase);
    g_free(sunrise);
    g_free(sunset);
    g_free(moonrise);
    g_free(moonset);
    return out;
}


gchar *
weather_dump_location(const xml_location *loc,
                      const gboolean interval)
{
    gchar *out;

    if (!loc)
        return g_strdup("No location data.");

    if (interval)
        out =
            g_strdup_printf("alt=%s, lat=%s, lon=%s, "
                            "prec=%s %s, symid=%d (%s)",
                            loc->altitude,
                            loc->latitude,
                            loc->longitude,
                            loc->precipitation_value,
                            loc->precipitation_unit,
                            loc->symbol_id,
                            loc->symbol);
    else
        out =
            g_strdup_printf("alt=%s, lat=%s, lon=%s, temp=%s %s, "
                            "wind=%s %s° %s m/s (%s bf), "
                            "hum=%s %s, press=%s %s, fog=%s, cloudiness=%s, "
                            "cl=%s, cm=%s, ch=%s)",
                            loc->altitude,
                            loc->latitude,
                            loc->longitude,
                            loc->temperature_value,
                            loc->temperature_unit,
                            loc->wind_dir_name,
                            loc->wind_dir_deg,
                            loc->wind_speed_mps,
                            loc->wind_speed_beaufort,
                            loc->humidity_value,
                            loc->humidity_unit,
                            loc->pressure_value,
                            loc->pressure_unit,
                            loc->fog_percent,
                            loc->clouds_percent[CLOUDS_PERC_CLOUDINESS],
                            loc->clouds_percent[CLOUDS_PERC_LOW],
                            loc->clouds_percent[CLOUDS_PERC_MED],
                            loc->clouds_percent[CLOUDS_PERC_HIGH]);
    return out;
}


gchar *
weather_dump_weatherdata(const xml_weather *weatherdata)
{
    GString *out;
    gchar *start, *end, *loc, *result;
    gboolean is_interval;
    guint i;

    out = g_string_sized_new(20480);
    g_string_assign(out, "Timeslices (local time): ");
    g_string_append_printf(out, "%d timeslices available (%d max, %d free).\n",
                           weatherdata->num_timeslices, MAX_TIMESLICE,
                           MAX_TIMESLICE - weatherdata->num_timeslices);
    for (i = 0; i < weatherdata->num_timeslices; i++) {
        start = weather_debug_strftime_t(weatherdata->timeslice[i]->start);
        end = weather_debug_strftime_t(weatherdata->timeslice[i]->end);
        is_interval = (gboolean) strcmp(start, end);
        loc = weather_dump_location(weatherdata->timeslice[i]->location,
                                    is_interval);
        g_string_append_printf(out, "  #%3d: [%s %s %s] %s\n",
                               i + 1, start, is_interval ? "-" : "=",
                               end, loc);
        g_free(start);
        g_free(end);
        g_free(loc);
    }
    /* Remove trailing newline */
    if (out->str[out->len - 1] == '\n')
        out->str[--out->len] = '\0';

    /* Free GString only and return its character data */
    result = out->str;
    g_string_free(out, FALSE);
    return result;
}


gchar *
weather_dump_plugindata(const xfceweather_data *data)
{
    GString *out;
    GtkOrientation orientation, panel_orientation;
    gchar *last_astro_update, *last_data_update, *last_conditions_update;
    gchar *result;

    last_astro_update = weather_debug_strftime_t(data->last_astro_update);
    last_data_update = weather_debug_strftime_t(data->last_data_update);
    last_conditions_update =
        weather_debug_strftime_t(data->last_conditions_update);

    out = g_string_sized_new(1024);
    g_string_assign(out, "xfce_weatherdata:\n");
    g_string_append_printf(out,
                           "  --------------------------------------------\n"
                           "  panel size: %d px\n"
                           "  plugin size: %d px\n"
                           "  panel orientation: %d\n"
                           "  plugin orientation: %d\n"
                           "  --------------------------------------------\n"
                           "  last astro update: %s\n"
                           "  last data update: %s\n"
                           "  last conditions update: %s\n"
                           "  --------------------------------------------\n"
                           "  latitude: %s\n"
                           "  longitude: %s\n"
                           "  location name: %s\n"
                           "  unit system: %d\n"
                           "  night time: %s\n"
                           "  --------------------------------------------\n"
                           "  proxy from env: %s\n"
                           "  proxy host: %s\n"
                           "  proxy port: %d\n"
                           "  saved proxy host: %s\n"
                           "  saved proxy port: %d\n"
                           "  --------------------------------------------\n"
                           "  animation transitions: %s\n"
                           "  forecast days: %d\n"
                           "  --------------------------------------------",
                           data->panel_size,
                           data->size,
                           data->panel_orientation,
                           data->orientation,
                           last_astro_update,
                           last_data_update,
                           last_conditions_update,
                           data->lat,
                           data->lon,
                           data->location_name,
                           data->unit_system,
                           YESNO(data->night_time),
                           YESNO(data->proxy_fromenv),
                           data->proxy_host,
                           data->proxy_port,
                           data->saved_proxy_host,
                           data->saved_proxy_port,
                           YESNO(data->animation_transitions),
                           data->forecast_days);
    g_free(last_astro_update);
    g_free(last_data_update);
    g_free(last_conditions_update);

    /* Free GString only and return its character data */
    result = out->str;
    g_string_free(out, FALSE);
    return result;
}
