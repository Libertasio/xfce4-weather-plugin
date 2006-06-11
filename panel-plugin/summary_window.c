/* vim: set expandtab ts=8 sw=4: */

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public      as published by
 *  the Free Software Foundation; either version 2 of the     , or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public      for more details.
 *
 *  You should have received a copy of the GNU General Public     
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "parsers.h"
#include "get_data.h"
#include "plugin.h"

#include "summary_window.h"
#include "translate.h"
#include "icon.h"

#define BORDER 8

#define APPEND_BTEXT(text) gtk_text_buffer_insert_with_tags(GTK_TEXT_BUFFER(buffer),\
        &iter, text, -1, btag, NULL);

#define APPEND_TEXT_ITEM_REAL(text) gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &iter, text, -1);

#define APPEND_TEXT_ITEM(text, item) value = g_strdup_printf("\t%s: %s %s\n",\
        text, get_data(data, item), get_unit(unit, item));\
        APPEND_TEXT_ITEM_REAL(value); g_free(value);

static GtkTooltips *tooltips = NULL;

static GtkWidget *
create_summary_tab (xml_weather *data,
                    units        unit)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;  
    GtkTextTag *btag;
    gchar *value, *date, *wind, *sun, *vis;
    GtkWidget *view, *frame, *scrolled;

    view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    frame = gtk_frame_new(NULL);
    scrolled = gtk_scrolled_window_new(NULL, NULL);

    gtk_container_add(GTK_CONTAINER(scrolled), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_set_border_width(GTK_CONTAINER(frame), BORDER);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(frame), scrolled);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buffer),
            &iter,
            0);
    btag = gtk_text_buffer_create_tag(buffer, NULL, "weight", PANGO_WEIGHT_BOLD, NULL);

    
    value = g_strdup_printf(_("Weather report for: %s.\n\n"), get_data(data, DNAM));
    APPEND_BTEXT (value);
    g_free(value);

    date = translate_lsup(get_data(data, LSUP));
    value = g_strdup_printf(_("Observation station located in %s\nLast update: %s.\n"),
            get_data(data, OBST), date ? date : get_data(data, LSUP));
    APPEND_TEXT_ITEM_REAL(value);

    if (date)
        g_free(date);
    
    g_free(value);

    APPEND_BTEXT (_("\nTemperature\n"));
    APPEND_TEXT_ITEM (_("Temperature"), TEMP);
    APPEND_TEXT_ITEM (_("Windchill"), FLIK);
    
    /* special case for TRANS because of translate_desc */
    value = g_strdup_printf("\t%s: %s\n",
        _("Description"), translate_desc(get_data(data, TRANS)));
    APPEND_TEXT_ITEM_REAL(value); 
    g_free(value);

    APPEND_TEXT_ITEM (_("Dew point"), DEWP);

    APPEND_BTEXT (_("\nWind\n"));
    wind = translate_wind_speed(get_data(data, WIND_SPEED), unit);
	value = g_strdup_printf("\t%s: %s\n", _("Speed"), wind);
    APPEND_TEXT_ITEM_REAL(value);
    g_free(wind);
    g_free(value);

    wind = translate_wind_direction(get_data(data, WIND_DIRECTION));
    value = g_strdup_printf("\t%s: %s\n",
            _("Direction"), wind ? wind : get_data(data, WIND_DIRECTION));
    if (wind)
        g_free(wind);
    APPEND_TEXT_ITEM_REAL(value);
    g_free(value);
    
    wind = translate_wind_speed(get_data(data, WIND_GUST), unit);
	value = g_strdup_printf("\t%s: %s\n", _("Gusts"), wind);
    APPEND_TEXT_ITEM_REAL(value);
    g_free(wind);
    g_free(value);

    APPEND_BTEXT (_("\nUV\n"));
    APPEND_TEXT_ITEM (_("Index"), UV_INDEX);
    value = g_strdup_printf("\t%s: %s\n",
        _("Risk"), translate_risk(get_data(data, UV_TRANS)));
    APPEND_TEXT_ITEM_REAL(value); 
	g_free(value);

    APPEND_BTEXT (_("\nAtmospheric pressure\n"));
    APPEND_TEXT_ITEM (_("Pressure"), BAR_R);

    value = g_strdup_printf("\t%s: %s\n",
            _("State"), translate_bard(get_data(data, BAR_D)));
    APPEND_TEXT_ITEM_REAL(value);
    g_free(value); 

    APPEND_BTEXT (_("\nSun\n"));

    sun = translate_time(get_data(data, SUNR));
    value = g_strdup_printf("\t%s: %s\n",
            _("Rise"), sun ? sun : get_data(data, SUNR));
    if (sun)
        g_free(sun);
    APPEND_TEXT_ITEM_REAL(value);
    g_free(value);

    sun = translate_time(get_data(data, SUNS));
    value = g_strdup_printf("\t%s: %s\n",
            _("Set"), sun ? sun : get_data(data, SUNS));
    if (sun)
        g_free(sun);
    APPEND_TEXT_ITEM_REAL(value);
    g_free(value);

    APPEND_BTEXT (_("\nOther\n"));
    APPEND_TEXT_ITEM (_("Humidity"), HMID);
	vis = translate_visibility(get_data(data, VIS), unit);
	value = g_strdup_printf("\t%s: %s\n", _("Visibility"), vis);
    APPEND_TEXT_ITEM_REAL(value);
	g_free(vis);
    g_free(value);
    
    return frame;
}

static GtkWidget *
make_forecast (xml_dayf *weatherdata,
               units     unit)
{
    GtkWidget *item_vbox, *temp_hbox, *icon_hbox, *label, *icon_d, *icon_n, *box_d, *box_n;
    GdkPixbuf *icon;
    gchar *str, *day, *wind;

    DBG ("this day %s", weatherdata->day);

    item_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(item_vbox), 6);

    
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    
    day = translate_day(get_data_f(weatherdata, WDAY));
        
    str = g_strdup_printf("<b>%s</b>", day ? day : get_data_f(weatherdata, WDAY));
    if (day)
        g_free(day);
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(item_vbox), label, FALSE, FALSE, 0);

    icon_hbox = gtk_hbox_new(FALSE, 0);
    
    icon = get_icon(item_vbox, get_data_f(weatherdata, ICON_D), GTK_ICON_SIZE_DIALOG);
    icon_d = gtk_image_new_from_pixbuf(icon);
    g_object_unref (icon);
    box_d = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(box_d), icon_d);
    
    icon = get_icon(item_vbox, get_data_f(weatherdata, ICON_N), GTK_ICON_SIZE_DIALOG);
    icon_n = gtk_image_new_from_pixbuf(icon);
    g_object_unref (icon);
    box_n = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(box_n), icon_n);

    if (G_UNLIKELY (!tooltips))
        tooltips = gtk_tooltips_new ();
    
    str = g_strdup_printf(_("Day: %s"), translate_desc(get_data_f(weatherdata, TRANS_D)));
    gtk_tooltips_set_tip(tooltips, box_d, str, NULL);
    g_free(str);
    str = g_strdup_printf(_("Night: %s"), translate_desc(get_data_f(weatherdata, TRANS_N)));
    gtk_tooltips_set_tip(tooltips, box_n, str, NULL);
    g_free(str);

    gtk_box_pack_start(GTK_BOX(icon_hbox), box_d, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(icon_hbox), box_n, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(item_vbox), icon_hbox, FALSE, FALSE, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Precipitation</b>"));
    gtk_box_pack_start(GTK_BOX(item_vbox), label, FALSE, FALSE, 0);
    
    temp_hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("%s %%", get_data_f(weatherdata, PPCP_D));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("%s %%", get_data_f(weatherdata, PPCP_N));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(item_vbox), temp_hbox, FALSE, FALSE, 0);


    label = gtk_label_new(NULL);    
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Temperature</b>"));
    gtk_box_pack_start(GTK_BOX(item_vbox), label, FALSE, FALSE, 0);


    temp_hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("<span foreground=\"red\">%s</span> %s", 
            get_data_f(weatherdata, TEMP_MAX), get_unit(unit, TEMP));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("<span foreground=\"blue\">%s</span> %s", 
            get_data_f(weatherdata, TEMP_MIN), get_unit(unit, TEMP));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(item_vbox), temp_hbox, FALSE, FALSE, 0);

     
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Wind</b>"));
    gtk_box_pack_start(GTK_BOX(item_vbox), label, FALSE, FALSE, 0);

    temp_hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    
    wind = translate_wind_direction(get_data_f(weatherdata, W_DIRECTION_D));
    gtk_label_set_markup(GTK_LABEL(label), wind ? wind : get_data_f(weatherdata, W_DIRECTION_D));
    if (wind)
        g_free(wind);
 
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new(NULL); 
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    
    wind = translate_wind_direction(get_data_f(weatherdata, W_DIRECTION_N));
    gtk_label_set_markup(GTK_LABEL(label), wind ? wind : get_data_f(weatherdata, W_DIRECTION_N));
    if (wind)
        g_free(wind);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(item_vbox), temp_hbox, FALSE, FALSE, 0);

    /* speed */
    
    temp_hbox = gtk_hbox_new(FALSE, 2);
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("%s %s", get_data_f(weatherdata, W_SPEED_D),
            get_unit(unit, WIND_SPEED));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    str = g_strdup_printf("%s %s", get_data_f(weatherdata, W_SPEED_N),
            get_unit(unit, WIND_SPEED));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(temp_hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(item_vbox), temp_hbox, FALSE, FALSE, 0);

    DBG ("Done");
    
    return item_vbox;
}

static GtkWidget *
create_forecast_tab (xml_weather *data,
                     units        unit)
{
    GtkWidget *widg = gtk_hbox_new(FALSE, 0);
    guint i;

    gtk_container_set_border_width(GTK_CONTAINER(widg), 6);

    if (data && data->dayf)
    {
        for (i = 0; i < XML_WEATHER_DAYF_N - 1; i++)
        {
            if (!data->dayf[i])
                break;

            DBG ("%s", data->dayf[i]->day);

            gtk_box_pack_start(GTK_BOX(widg), 
                    make_forecast(data->dayf[i], unit), FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(widg),
                    gtk_vseparator_new(), TRUE, TRUE, 0);
        }

        if (data->dayf[i])
            gtk_box_pack_start(GTK_BOX(widg), 
                    make_forecast(data->dayf[i], unit), FALSE, FALSE, 0);
    }
    
    return widg;
}

GtkWidget *
create_summary_window (xml_weather *data,
                       units        unit)
{
    GtkWidget *window, *notebook, *vbox;
#ifdef USE_NEW_DIALOG
    gchar *title;
#else
    GtkWidget *header;
#endif
      	GdkPixbuf *icon;

#ifdef USE_NEW_DIALOG
    window = xfce_titled_dialog_new_with_buttons (_("Weather Update"),
                          NULL,
                          GTK_DIALOG_NO_SEPARATOR,
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);
    
    title = g_strdup_printf(_("Weather report for: %s"), get_data(data, DNAM));

    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (window),
                     title);
    g_free (title);
#else
    window = gtk_dialog_new_with_buttons(_("Weather Update"), NULL,
            0,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
#endif
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), vbox, TRUE, TRUE, 0); 

    icon = get_icon (window, get_data(data, WICON), GTK_ICON_SIZE_DIALOG);
#ifdef USE_NEW_DIALOG
    gtk_window_set_icon (GTK_WINDOW (window), icon);
#else
    header = xfce_create_header(icon, _("Weather Update")); 
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);
#endif

    g_object_unref (icon);

    notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), BORDER);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
            create_summary_tab(data, unit), gtk_label_new(_("Summary")));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
            create_forecast_tab(data, unit), gtk_label_new(_("Forecast")));

    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    g_signal_connect(window, "response",
            G_CALLBACK (gtk_widget_destroy), window);

    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);

    return window;
}
