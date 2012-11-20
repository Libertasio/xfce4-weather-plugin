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

#include <string.h>
#include <libxfce4ui/libxfce4ui.h>

#include "weather-parsers.h"
#include "weather-data.h"
#include "weather.h"

#include "weather-config.h"
#include "weather-search.h"
#include "weather-scrollbox.h"

#define OPTIONS_N 13
#define BORDER 8
#define LOC_NAME_MAX_LEN 50


static const labeloption labeloptions[OPTIONS_N] = {
    /*
     * TRANSLATORS: The abbreviations in parentheses will be shown in
     * the scrollbox together with the values. Keep them in sync with
     * those in make_label() in weather.c. Some of them may be
     * standardized internationally, like CL, CM, CH, and you might
     * read that up somewhere and decide whether you want to use them
     * or not. In general, though, you should just try to choose
     * letter(s) that make sense and don't use up too much space.
     */
    {N_("Temperature (T)"), TEMPERATURE},
    {N_("Atmosphere pressure (P)"), PRESSURE},
    {N_("Wind speed (WS)"), WIND_SPEED},
    {N_("Wind speed - Beaufort scale (WB)"), WIND_BEAUFORT},
    {N_("Wind direction (WD)"), WIND_DIRECTION},
    {N_("Wind direction in degrees (WD)"), WIND_DIRECTION_DEG},
    {N_("Humidity (H)"), HUMIDITY},
    {N_("Low clouds (CL)"), CLOUDS_LOW},
    {N_("Medium clouds (CM)"), CLOUDS_MED},
    {N_("High clouds (CH)"), CLOUDS_HIGH},
    {N_("Cloudiness (C)"), CLOUDINESS},
    {N_("Fog (F)"), FOG},
    {N_("Precipitations (R)"), PRECIPITATIONS},
};

typedef void (*cb_function) (xfceweather_data *);
static cb_function cb = NULL;


static void
add_mdl_option(GtkListStore *mdl,
               const gint opt)
{
    GtkTreeIter iter;

    gtk_list_store_append(mdl, &iter);
    gtk_list_store_set(mdl, &iter,
                       0, _(labeloptions[opt].name),
                       1, labeloptions[opt].number, -1);
}


static gboolean
cb_addoption(GtkWidget *widget,
             gpointer data)
{
    xfceweather_dialog *dialog;
    gint history;

    dialog = (xfceweather_dialog *) data;
    history =
        gtk_option_menu_get_history(GTK_OPTION_MENU(dialog->opt_xmloption));
    add_mdl_option(dialog->mdl_xmloption, history);

    return FALSE;
}


static gboolean
cb_deloption(GtkWidget *widget,
             gpointer data)
{
    xfceweather_dialog *dialog = (xfceweather_dialog *) data;
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->lst_xmloption));
    if (gtk_tree_selection_get_selected(selection, NULL, &iter))
        gtk_list_store_remove(GTK_LIST_STORE(dialog->mdl_xmloption), &iter);

    return FALSE;
}


static gboolean
cb_upoption(GtkWidget *widget,
            gpointer data)
{
    xfceweather_dialog *dialog = (xfceweather_dialog *) data;
    GtkTreeSelection *selection;
    GtkTreeIter iter, prev;
    GtkTreePath *path;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->lst_xmloption));
    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(dialog->mdl_xmloption),
                                       &iter);
        if (gtk_tree_path_prev(path)) {
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->mdl_xmloption),
                                        &prev, path))
                gtk_list_store_move_before(GTK_LIST_STORE(dialog->mdl_xmloption),
                                           &iter, &prev);

            gtk_tree_path_free(path);
        }
    }

    return FALSE;
}


static gboolean
cb_downoption(GtkWidget *widget,
              gpointer data)
{
    xfceweather_dialog *dialog = (xfceweather_dialog *) data;
    GtkTreeIter iter, next;
    GtkTreeSelection *selection;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->lst_xmloption));
    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        next = iter;
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(dialog->mdl_xmloption),
                                     &next))
            gtk_list_store_move_after(GTK_LIST_STORE(dialog->mdl_xmloption),
                                      &iter, &next);
    }

    return FALSE;
}


static gboolean
cb_toggle(GtkWidget *widget,
          gpointer data)
{
    GtkWidget *target = (GtkWidget *) data;

    gtk_widget_set_sensitive(target,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (widget)));
    return FALSE;
}


static gboolean
cb_not_toggle(GtkWidget *widget,
              gpointer data)
{
    GtkWidget *target = (GtkWidget *) data;

    gtk_widget_set_sensitive(target,
                             !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                           (widget)));
    return FALSE;
}


static GtkWidget *
make_label(void)
{
    GtkWidget *widget, *menu;
    guint i;

    menu = gtk_menu_new();
    widget = gtk_option_menu_new();
    for (i = 0; i < OPTIONS_N; i++) {
        labeloption opt = labeloptions[i];

        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                              gtk_menu_item_new_with_label(_(opt.name)));
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(widget), menu);
    return widget;
}


static gchar *
sanitize_location_name(const gchar *location_name)
{
    gchar *pos, *pos2, sane[LOC_NAME_MAX_LEN * 4];
    glong len, offset;

    pos = g_utf8_strchr(location_name, -1, ',');
    if (pos != NULL) {
        pos2 = pos;
        while ((pos2 = g_utf8_next_char(pos2)))
            if (g_utf8_get_char(pos2) == ',') {
                pos = pos2;
                break;
            }
        offset = g_utf8_pointer_to_offset(location_name, pos);
        if (offset > LOC_NAME_MAX_LEN)
            offset = LOC_NAME_MAX_LEN;
        (void) g_utf8_strncpy(sane, location_name, offset);
        sane[LOC_NAME_MAX_LEN * 4 - 1] = '\0';
        return g_strdup(sane);
    } else {
        len = g_utf8_strlen(location_name, LOC_NAME_MAX_LEN);

        if (len >= LOC_NAME_MAX_LEN) {
            (void) g_utf8_strncpy(sane, location_name, len);
            sane[LOC_NAME_MAX_LEN * 4 - 1] = '\0';
            return g_strdup(sane);
        }

        if (len > 0)
            return g_strdup(location_name);

        return g_strdup(_("Unset"));
    }
}


void
apply_options(xfceweather_dialog *dialog)
{
    gint option;
    gboolean hasiter = FALSE;
    GtkTreeIter iter;
    gchar *text;
    GValue value = { 0, };
    GtkWidget *widget;
    xfceweather_data *data = (xfceweather_data *) dialog->wd;

    option = gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->combo_unit_system));
    if (option == IMPERIAL)
        data->unit_system = IMPERIAL;
    else
        data->unit_system = METRIC;

    if (data->lat)
        g_free(data->lat);
    if (data->lon)
        g_free(data->lon);
    if (data->location_name)
        g_free(data->location_name);

    data->lat =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->txt_lat)));
    data->lon =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->txt_lon)));
    data->location_name =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->txt_loc_name)));

    /* force update of astronomical data */
    memset(&data->last_astro_update, 0, sizeof(data->last_astro_update));

    /* call labels_clear() here */
    if (data->labels && data->labels->len > 0)
        g_array_free(data->labels, TRUE);

    data->labels = g_array_new(FALSE, TRUE, sizeof(data_types));
    for (hasiter =
             gtk_tree_model_get_iter_first(GTK_TREE_MODEL
                                           (dialog->mdl_xmloption),
                                           &iter);
         hasiter == TRUE;
         hasiter =
             gtk_tree_model_iter_next(GTK_TREE_MODEL(dialog->mdl_xmloption),
                                      &iter)) {
        gtk_tree_model_get_value(GTK_TREE_MODEL(dialog->mdl_xmloption), &iter,
                                 1, &value);
        option = g_value_get_int(&value);
        g_array_append_val(data->labels, option);
        g_value_unset(&value);
    }

    data->forecast_days = 
        (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON
                                         (dialog->spin_forecast_days));

    data->animation_transitions =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (dialog->chk_animate_transition));
    gtk_scrollbox_set_animate(GTK_SCROLLBOX(data->scrollbox),
                              data->animation_transitions);

    if (cb)
        cb(data);
}


static int
option_i(const data_types opt)
{
    guint i;

    for (i = 0; i < OPTIONS_N; i++)
        if (labeloptions[i].number == opt)
            return i;

    return -1;
}


static void
set_location_tooltip(xfceweather_dialog *dialog,
                     const gchar *lat,
                     const gchar *lon)
{
    gchar *text;

    if (lat && lon)
        text = g_strdup_printf
            (_("Latitude: %s, Longitude: %s\n\n"
               "You may edit the location name to your liking.\n"
               "To choose another location, "
               "please use the \"Change\" button."),
             lat, lon);
    else
        text = g_strdup(_("Please select a location "
                          "by using the \"Change\" button."));
    gtk_widget_set_tooltip_text(dialog->txt_loc_name, text);
    g_free(text);
}


static void
auto_locate_cb(const gchar *loc_name,
               const gchar *lat,
               const gchar *lon,
               const unit_systems unit_system,
               gpointer user_data)
{
    xfceweather_dialog *dialog = (xfceweather_dialog *) user_data;

    if (lat && lon && loc_name) {
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lat), lat);
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lon), lon);
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name), loc_name);
        gtk_widget_set_sensitive(dialog->txt_loc_name, TRUE);
        set_location_tooltip(dialog, lat, lon);
    } else {
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lat), "");
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lon), "");
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name), _("Unset"));
        gtk_widget_set_sensitive(dialog->txt_loc_name, FALSE);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->combo_unit_system),
                             unit_system);
}


static void
start_auto_locate(xfceweather_dialog *dialog)
{
    gtk_widget_set_sensitive(dialog->txt_loc_name, FALSE);
    gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name), _("Detecting..."));
    weather_search_by_ip(dialog->wd->session, auto_locate_cb, dialog);
}


static gboolean
cb_findlocation(GtkButton *button,
                gpointer user_data)
{
    xfceweather_dialog *dialog = (xfceweather_dialog *) user_data;
    search_dialog *sdialog;
    gchar *loc_name;

    sdialog = create_search_dialog(NULL, dialog->wd->session);

    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
    if (run_search_dialog(sdialog)) {
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lat), sdialog->result_lat);
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lon), sdialog->result_lon);
        loc_name = sanitize_location_name(sdialog->result_name);
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name), loc_name);
        g_free(loc_name);
        gtk_widget_set_sensitive(dialog->txt_loc_name, TRUE);
        set_location_tooltip(dialog, sdialog->result_lat, sdialog->result_lon);
    }
    free_search_dialog(sdialog);
    gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);

    return FALSE;
}


xfceweather_dialog *
create_config_dialog(xfceweather_data *data,
                     GtkWidget *vbox)
{
    xfceweather_dialog *dialog;
    GtkWidget *vbox2, *vbox3, *hbox, *hbox2, *label;
    GtkWidget *button_add, *button_del, *button_up, *button_down;
    GtkWidget *image, *button, *scroll;
    GtkSizeGroup *sg, *sg_buttons;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    data_types type;
    guint i;
    gint n;

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    sg_buttons = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    dialog = g_slice_new0(xfceweather_dialog);
    dialog->wd = (xfceweather_data *) data;
    dialog->dialog = gtk_widget_get_toplevel(vbox);

    /* system of measurement */
    label = gtk_label_new_with_mnemonic(_("System of _Measurement:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    dialog->combo_unit_system = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_unit_system),
                              _("Imperial"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_unit_system),
                              _("Metric"));
    gtk_label_set_mnemonic_widget(GTK_LABEL(label),
                                  GTK_WIDGET(dialog->combo_unit_system));
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->combo_unit_system),
                             dialog->wd->unit_system);
    hbox = gtk_hbox_new(FALSE, BORDER);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), dialog->combo_unit_system, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_size_group_add_widget(sg, label);

    /* location */
    label = gtk_label_new_with_mnemonic(_("L_ocation:"));
    dialog->txt_lat = gtk_entry_new();
    dialog->txt_lon = gtk_entry_new();
    dialog->txt_loc_name = gtk_entry_new();
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label),
                                  GTK_WIDGET(dialog->txt_loc_name));
    if (dialog->wd->lat != NULL && strlen(dialog->wd->lat) > 0)
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lat),
                           dialog->wd->lat);
    else
        gtk_widget_set_sensitive(dialog->txt_loc_name, FALSE);
    if (dialog->wd->lon != NULL && strlen(dialog->wd->lon) > 0)
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_lon),
                           dialog->wd->lon);
    else
        gtk_widget_set_sensitive(dialog->txt_loc_name, FALSE);
    if (dialog->wd->location_name != NULL)
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name),
                           dialog->wd->location_name);
    else
        gtk_entry_set_text(GTK_ENTRY(dialog->txt_loc_name), _("Unset"));
    gtk_entry_set_max_length(GTK_ENTRY(dialog->txt_loc_name), LOC_NAME_MAX_LEN);
    set_location_tooltip(dialog, dialog->wd->lat, dialog->wd->lon);
    if ((dialog->wd->lat == NULL || dialog->wd->lon == NULL) ||
        (! strlen(dialog->wd->lat) || ! strlen(dialog->wd->lon)))
        start_auto_locate(dialog);
    gtk_size_group_add_widget(sg, label);
    button = gtk_button_new_with_mnemonic(_("Chan_ge..."));
    image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_findlocation), dialog);
    hbox = gtk_hbox_new(FALSE, BORDER);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), dialog->txt_loc_name, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* number of days shown in forecast */
    label = gtk_label_new_with_mnemonic(_("Number of _forecast days:"));
    dialog->spin_forecast_days =
        gtk_spin_button_new_with_range(1, MAX_FORECAST_DAYS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_forecast_days),
                              (dialog->wd->forecast_days)
                              ? dialog->wd->forecast_days : 5);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label),
                                  GTK_WIDGET(dialog->spin_forecast_days));
    hbox = gtk_hbox_new(FALSE, BORDER);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       dialog->spin_forecast_days, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_size_group_add_widget(sg, label);

    /* labels and buttons */
    dialog->opt_xmloption = make_label();
    dialog->mdl_xmloption = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    dialog->lst_xmloption =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog->mdl_xmloption));
    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(_("_Labels to display"),
                                                 renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->lst_xmloption), column);

    /* button "add" */
    button_add = gtk_button_new_with_mnemonic(_("_Add"));
    image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_add), image);
    gtk_size_group_add_widget(sg_buttons, button_add);
    hbox = gtk_hbox_new(FALSE, BORDER);
    gtk_box_pack_start(GTK_BOX(hbox), dialog->opt_xmloption, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_add, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    hbox = gtk_hbox_new(FALSE, BORDER);
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), dialog->lst_xmloption);
    gtk_box_pack_start(GTK_BOX(hbox), scroll, TRUE, TRUE, 0);

    /* button "remove" */
    button_del = gtk_button_new_with_mnemonic(_("_Remove"));
    image = gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_del), image);
    gtk_size_group_add_widget(sg_buttons, button_del);

    /* button "move up" */
    button_up = gtk_button_new_with_mnemonic(_("Move _Up"));
    image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_up), image);
    gtk_size_group_add_widget(sg_buttons, button_up);

    /* button "move down" */
    button_down = gtk_button_new_with_mnemonic(_("Move _Down"));
    image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_down), image);
    gtk_size_group_add_widget(sg_buttons, button_down);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), button_del, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), button_up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), button_down, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_set_size_request(dialog->lst_xmloption, -1, 120);

    if (data->labels->len > 0) {
        for (i = 0; i < data->labels->len; i++) {
            type = g_array_index(data->labels, data_types, i);

            if ((n = option_i(type)) != -1)
                add_mdl_option(dialog->mdl_xmloption, n);
        }
    }

    g_object_unref(G_OBJECT(sg));
    g_object_unref(G_OBJECT(sg_buttons));

    g_signal_connect(G_OBJECT(button_add), "clicked",
                     G_CALLBACK(cb_addoption), dialog);
    g_signal_connect(G_OBJECT(button_del), "clicked",
                     G_CALLBACK(cb_deloption), dialog);
    g_signal_connect(G_OBJECT(button_up), "clicked",
                     G_CALLBACK(cb_upoption), dialog);
    g_signal_connect(G_OBJECT(button_down), "clicked",
                     G_CALLBACK(cb_downoption), dialog);

    dialog->chk_animate_transition =
        gtk_check_button_new_with_mnemonic(_("Animate _transitions between labels"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (dialog->chk_animate_transition),
                                 dialog->wd->animation_transitions);
    gtk_box_pack_start(GTK_BOX(vbox), dialog->chk_animate_transition,
                       FALSE, FALSE, 0);

    gtk_widget_show_all(vbox);

    return dialog;
}


void
set_callback_config_dialog(xfceweather_dialog *dialog,
                           cb_function cb_new)
{
    cb = cb_new;
}
