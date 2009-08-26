/*
 * File:   democlient-callbacks.h
 * Author: damquang.tuan@nomovok.com
 *
 * Created on August 26, 2009, 10:04 AM
 */
#include <gtk/gtk.h>

/**
 * create audio sink for the pipeline
 * This sink is used in the decoding progress
 * The name of return object is "audiosink"
 *
 * @param None
 *
 * @return GstElement* the audio sink or NULL if something wrong
 *
 * @author Dam Quang Tuan <damquang.tuan@nomovok.com>
 */
void
democlient_on_btn_Connect_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Disconnect_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Pause_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Resume_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Options_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_About_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Quit_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_ConnectDialog_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
democlient_on_btn_Disconnect_clicked              (GtkButton       *button,
                                        gpointer         user_data);