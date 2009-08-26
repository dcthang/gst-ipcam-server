/*
 * File:   democlient-interface.h
 * Author: damquang.tuan@nomovok.com
 *
 * Created on August 26, 2009, 10:04 AM
 */
#include <gtk/gtk.h>

GtkWidget *mainWindow;
GtkWidget *vbox;
GtkWidget *toolbar;
GtkIconSize tmp_toolbar_icon_size;
GtkWidget *toolitem_Connect;
GtkWidget *btn_Connect;
GtkWidget *btn_Disconnect;

GtkWidget *toolitem_Pause;
GtkWidget *btn_Pause;
GtkWidget *btn_Resume;
GtkWidget *img_Resume;
GtkWidget *algn_Resume;
GtkWidget *hbox_ResumeButton;
GtkWidget *lbl_ResumeButton;
GtkWidget *toolitem_Options;
GtkWidget *btn_Options;
GtkWidget *img_Options;
GtkWidget *algn_Options;
GtkWidget *hbox_OptionsButton;
GtkWidget *lbl_OptionsButton;
GtkWidget *toolitem_About;
GtkWidget *btn_About;
GtkWidget *toolitem_Quit;
GtkWidget *btn_Quit;
GtkWidget *hbox_VideoPrew;
GtkWidget *prw_GuestVideo;
GtkWidget *prw_HostVideo;
GtkWidget *statusBar;

GtkWidget* democlient_create_mainWindow (void);
GtkWidget* democlient_create_connectionDialog (void);
GtkWidget* democlient_create_optionDialog (void);