/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+
#+     Glade / Gtk Programming
#+
#+     Copyright (C) 2019 by Kevin C. O'Kane
#+
#+     Kevin C. O'Kane
#+     kc.okane@gmail.com
#+     https://www.cs.uni.edu/~okane
#+     http://threadsafebooks.com/
#+
#+ This program is free software; you can redistribute it and/or modify
#+ it under the terms of the GNU General Public License as published by
#+ the Free Software Foundation; either version 2 of the License, or
#+ (at your option) any later version.
#+
#+ This program is distributed in the hope that it will be useful,
#+ but WITHOUT ANY WARRANTY; without even the implied warranty of
#+ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#+ GNU General Public License for more details.
#+
#+ You should have received a copy of the GNU General Public License
#+ along with this program; if not, write to the Free Software
#+ Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#+
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//#include "gpioTest.h"
#include "UART_test.h"
#include "waveForm.h"
#include "ADS1x15.h"
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gst/gst.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/mman.h>
#include <stdio.h>
#include <linux/limits.h>
#include <wiringPi.h>
#include <pigpio.h>
#include <pthread.h>
#include <signal.h>
//#include <raspicam/raspicam.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video.h>
//#include <gst/interfaces/xoverlay.h>
#include <Python.h>
#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined(GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

//#include "gpioTest.h"

#define LEFT_BUTTON 22
#define MIDDLE_BUTTON 27
#define RIGHT_BUTTON 17

GtkWidget *window;
GtkWidget *fixed1;
GtkWidget *left_label;
GtkWidget *middle_label;
GtkWidget *right_label;
GtkWidget *status_label;
GtkWidget *bat_label;
GtkWidget *eventbox_label_l;
GtkWidget *eventbox_label_m;
GtkWidget *eventbox_label_r;
GtkWidget *eventbox_status;
GtkWidget *eventbox_ppm;
GtkWidget *date_label;
GtkWidget *time_label;
GtkWidget *ppm_display_label;
GtkWidget *video_screen;
GtkWidget *splash_screen;
GtkWidget *laser_on, *laser_off, *gps_on, *gps_off;
GtkWidget *setup_fields_labels;
GtkWidget *setup_fields_values;
GtkWidget *info_laser, *info_gps, *info_bat;
GtkWidget *setup_label_1, *setup_label_2, *setup_label_3, *setup_label_4, *setup_label_5, *setup_label_6, *setup_label_7;
GtkTextView *setup_value_1, *setup_value_2, *setup_value_3, *setup_value_4, *setup_value_5, *setup_value_6, *setup_value_7;

GtkBuilder *builder;

GstElement *pipeline, *source, *sink, *convert, *filter, *flip;
GstCaps *videosrc_caps;
GstBus *bus;
GstMessage *msg;
GstStateChangeReturn ret;
gboolean status = 0;

GdkWindow *video_window_xwindow;
gulong embed_xid;
GstStateChangeReturn sret;

GstSample *from_sample, *to_sample;
GstCaps *image_caps;
GstBuffer *buf;
GstMapInfo map_info;
GError *err = NULL;

void on_destroy();
enum Mode
{
	Splash = 0,
	Setup,
	Idle,
	BarGraph,
	PPM,
	LiveCam,
	IRCam,
	Shutdown
} OpMode;

const gchar *labelstring, *setup_buf;
time_t current_time;
struct tm *time_info;
int current_min = -1;
int ppm = 0;
int cam = 0;
long int timedate;
int counter = 0;
float dist = 0;

typedef struct measData_t{
	int ppm;  //count
	float ADVoltag;
	float dist;
	int wid;
}measData;

measData mData; ads1x15_p adcMain;

GMutex mutex_1, mutex_2, mutex_3;

/**
	 * C++ version 0.4 char* style "itoa":
	 * Written by Lukás Chmela
	 * Released under GPLv3.

	 */
char *itoa(int value, char *result, int base)
{
	// check that the base if valid
	if (base < 2 || base > 36)
	{
		*result = '\0';
		return result;
	}

	char *ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

void on_destroy()
{
	gtk_main_quit();
}

void cleanup(int signo)
{
	pullUpDnControl(LEFT_BUTTON, PUD_DOWN);
	pullUpDnControl(MIDDLE_BUTTON, PUD_DOWN);
	pullUpDnControl(RIGHT_BUTTON, PUD_DOWN);
 
	exit(0);
}

char *call_Python_QR(int argc, char *argv1, char *argv2)
{
	PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *pmyresult;
	char *l_UserID;
	// Set PYTHONPATH TO working directory
	setenv("PYTHONPATH", ".", 1);

	wchar_t *program = Py_DecodeLocale(argv1, NULL);
	if (program == NULL)
	{
		fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
		exit(1);
	}
	Py_SetProgramName(program); /* optional but recommended */
	// Initialize the Python Interpreter
	Py_Initialize();

	// Build the name object
	pName = PyUnicode_DecodeFSDefault((char *)argv1);

	// Load the module object
	pModule = PyImport_Import(pName);

	// pDict is a borrowed reference
	pDict = PyModule_GetDict(pModule);

	// pFunc is also a borrowed reference
	pFunc = PyDict_GetItemString(pDict, argv2);

	if (PyCallable_Check(pFunc))
	{
		pmyresult = PyObject_CallObject(pFunc, NULL);
	}
	else
	{
		PyErr_Print();
	}
	if (PyUnicode_Check(pmyresult))
	{
		PyObject *temp_bytes = PyUnicode_AsEncodedString(pmyresult, "UTF-8", "strict"); // Owned reference
		if (temp_bytes != NULL)
		{
			l_UserID = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
			l_UserID = strdup(l_UserID);
			Py_DECREF(temp_bytes);
		}
		else
		{
			// TODO: Handle encoding error.
		}
	}

	// Clean up
	Py_DECREF(pModule);
	Py_DECREF(pName);

	// Finish the Python Interpreter
	Py_Finalize();

	return l_UserID;
}

int call_Python_Stitch(int argc, char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
	PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *pmyresult, *args, *kwargs;
	int i;
	// Set PYTHONPATH TO working directory
	setenv("PYTHONPATH", ".", 1);

	wchar_t *program = Py_DecodeLocale(argv1, NULL);
	if (program == NULL)
	{
		fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
		exit(1);
	}
	Py_SetProgramName(program); /* optional but recommended */
	// Initialize the Python Interpreter
	Py_Initialize();

	// Build the name object
	pName = PyUnicode_DecodeFSDefault(argv1);


	// Load the module object
	pModule = PyImport_Import(pName);

	// pDict is a borrowed reference
	pDict = PyModule_GetDict(pModule);

	// pFunc is also a borrowed reference
	pFunc = PyDict_GetItemString(pDict, argv2);

	//args = PyTuple_Pack(2,PyUnicode_DecodeFSDefault(argv3), PyUnicode_DecodeFSDefault(argv4));
	//kwargs = PyTuple_Pack(2,PyUnicode_DecodeFSDefault(argv5), PyUnicode_DecodeFSDefault(argv6));
	args = Py_BuildValue("ssss", argv5,argv3, argv6, argv4);
	//kwargs = Py_BuildValue("ss", argv5, argv6);
	if (PyCallable_Check(pFunc))
	{
		pmyresult = PyObject_CallObject(pFunc, NULL);
		i = 0;
	}
	else
	{
		PyErr_Print();
		i = 1;
	}
	

	// Clean up
	Py_DECREF(pModule);
	Py_DECREF(pName);

	// Finish the Python Interpreter
	Py_Finalize();

	return i;
}

void setup_filestructure()
{
	GtkTextIter start, end;
	GtkTextBuffer *userID;
	gchar *text;
	userID = gtk_text_view_get_buffer(setup_value_7);
	gtk_text_buffer_get_bounds(userID, &start, &end);
	text = gtk_text_buffer_get_text(userID, &start, &end, FALSE);
	if (strcmp(text, "") == 0)
	{
		text = call_Python_QR(2, "Decode_QR", "decode");
	}
	gtk_text_buffer_set_text(userID, text, strlen(text));
	gtk_text_view_set_buffer(setup_value_7, userID);
}

void left_button_pressed()
{

	labelstring = gtk_label_get_text(GTK_LABEL(left_label));
	if (strcmp(labelstring, "Setup") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Return");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Save");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Setup");
		gtk_widget_hide(eventbox_ppm);
		gtk_widget_hide(ppm_display_label);
		gtk_widget_hide(video_screen);
		gtk_widget_show(setup_fields_labels);
		gtk_widget_show(setup_fields_values);
		OpMode = Setup;
	}
	else if (strcmp(labelstring, "Return") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Setup");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Start");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Exit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Idle");
		gtk_widget_hide(setup_fields_labels);
		gtk_widget_hide(setup_fields_values);
		OpMode = Idle;
	}
	else if (strcmp(labelstring, "Yes") == 0)
	{
		labelstring = gtk_label_get_text(GTK_LABEL(status_label));
		if (strcmp(labelstring, "Confirm Exit") == 0)
		{
			gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Bye");
			OpMode = Shutdown;
		}
	}
	else if (strcmp(labelstring, "PPM/DIST") == 0)
	{
		gtk_widget_show(ppm_display_label);
		gtk_widget_show(eventbox_ppm);
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Quit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Survey PPM");
		mData.wid = wavePiset();
		OpMode = PPM;
	}
	else if (strcmp(labelstring, "IR Cam") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Quit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"IR Camera");
		OpMode = IRCam;
	}
}

void middle_button_pressed()
{
	labelstring = gtk_label_get_text(GTK_LABEL(middle_label));
	if (strcmp(labelstring, "Start") == 0)
	{
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Startup");
		// call startup sequence here
		{
			delay(1000);
		}
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Choose Mode");
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"PPM/DIST");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Live Cam");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Bar Graph");
	}
	else if (strcmp(labelstring, "Live Cam") == 0)
	{
		gtk_widget_hide(ppm_display_label);
		gtk_widget_hide(eventbox_ppm);
		// gtk_label_set_text(GTK_LABEL(ppm_display_label), (const gchar* ) "");
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"IR Cam");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Snapshot");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Quit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Survey Live");
		OpMode = LiveCam;
	}
	else if (strcmp(labelstring, "Snapshot") == 0)
	{
		g_object_get(sink, "last-sample", &from_sample, NULL);
		if (from_sample == NULL)
		{
			GST_ERROR("Error getting last sample form sink");
			return;
		}
		image_caps = gst_caps_from_string("image/png");
		to_sample = gst_video_convert_sample(from_sample, image_caps, GST_CLOCK_TIME_NONE, &err);
		gst_caps_unref(image_caps);
		gst_sample_unref(from_sample);

		if (to_sample == NULL && err)
		{
			GST_ERROR("Error converting frame: %s", err->message);
			g_printerr("Error converting frame. \n");
			g_error_free(err);
			return;
		}
		buf = gst_sample_get_buffer(to_sample);
		if (gst_buffer_map(buf, &map_info, GST_MAP_READ))
		{
			if (!g_file_set_contents("Snap.png", (const char *)map_info.data,
									 map_info.size, &err))
			{
				GST_CAT_LEVEL_LOG(GST_CAT_DEFAULT, GST_LEVEL_WARNING, NULL, "Could not save thumbnail: %s", err->message);
				g_error_free(err);
			}
		}
	}
	else if (strcmp(labelstring, "Edit") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Return");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Save");
		gtk_text_view_set_editable(setup_value_1, True);
		gtk_text_view_set_cursor_visible(setup_value_1, True);
	}
}

void right_button_pressed()
{

	labelstring = gtk_label_get_text(GTK_LABEL(right_label));
	if (strcmp(labelstring, "Exit") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Yes");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"No");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Confirm Exit");
	}
	else if (strcmp(labelstring, "No") == 0)
	{
		labelstring = gtk_label_get_text(GTK_LABEL(status_label));
		if (strcmp(labelstring, "Confirm Exit") == 0)
		{
			gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Setup");
			gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Start");
			gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Exit");
			gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Idle");
			OpMode = Idle;
		}
	}
	else if (strcmp(labelstring, "IR Cam") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Quit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Survey IR");
		OpMode = IRCam;
	}
	else if (strcmp(labelstring, "Quit") == 0)
	{
		gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Setup");
		gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Start");
		gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Exit");
		gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Idle");
		OpMode = Idle;
		if(mData.wid >= 0)
		{
			wavePistop(mData.wid);
			mData.wid = -1;
		}
	}
	else if (strcmp(labelstring, "Save") == 0)
	{
	}
}

gboolean update_ppm(gpointer ppm)
{
	char buffer[10];

	g_mutex_lock(&mutex_1);
	// update the GUI here:
	// gtk_button_set_label(button,"label");
	itoa((int)ppm, buffer, 10);
	//itoa((float)ppm, buffer, 10);
	strcat(buffer, " PPM");
	strcat(buffer, "\n\r 8.0 M");
	gtk_label_set_text(GTK_LABEL(ppm_display_label), buffer);

	// And read the GUI also here, before the mutex to be unlocked:
	// gchar * text = gtk_entry_get_text(GTK_ENTRY(entry));
	g_mutex_unlock(&mutex_1);

	return FALSE;
}

gboolean update_meas(gpointer mData)
{
	char buffer[20], buffer1[10];
	measData lmData = *(measData*)mData;

	g_mutex_lock(&mutex_1);
	// update the GUI here:
	// gtk_button_set_label(button,"label");
	//itoa((int)ppm, buffer, 10);
	//itoa(lmData.ppm, buffer, 10);
	sprintf(buffer, "%d", lmData.ppm);
	strcat(buffer, " PPM\n\r");
	sprintf(buffer1, "%2.2f", lmData.dist);
	strcat(buffer1, " M");
	sprintf(buffer, "%2.2f", lmData.ADVoltag);
	strcat(buffer, " V\n\r");
	strcat(buffer, buffer1);
	gtk_label_set_text(GTK_LABEL(ppm_display_label), buffer);

	// And read the GUI also here, before the mutex to be unlocked:
	// gchar * text = gtk_entry_get_text(GTK_ENTRY(entry));
	g_mutex_unlock(&mutex_1);

	return FALSE;
}


gboolean update_time(gpointer time_info)
{
	char dateString[9];
	char timeString[9];
	g_mutex_lock(&mutex_2);
	// update the GUI here:
	// gtk_button_set_label(button,"label");

	//char *cp = calloc(1, sizeof(dateString));//test

	strftime(dateString, sizeof(dateString), "%D", time_info);
	gtk_label_set_text(GTK_LABEL(date_label), dateString);
	strftime(timeString, sizeof(timeString), "%I:%M %p", time_info);
	gtk_label_set_text(GTK_LABEL(time_label), timeString);

	// And read the GUI also here, before the mutex to be unlocked:
	// gchar * text = gtk_entry_get_text(GTK_ENTRY(entry));
	g_mutex_unlock(&mutex_2);

	return FALSE;
}

gboolean live_stream(gpointer pipeline)
{
	char dateString[9];
	char timeString[9];
	GstStateChangeReturn sret;
	g_mutex_lock(&mutex_3);
	// update the GUI here:
	// gtk_button_set_label(button,"label");

	strftime(dateString, sizeof(dateString), "%D", time_info);
	gtk_label_set_text(GTK_LABEL(date_label), dateString);
	strftime(timeString, sizeof(timeString), "%I:%M %p", time_info);
	gtk_label_set_text(GTK_LABEL(time_label), timeString);

	if (status == 0)
	{
		/* run the pipeline */
		gtk_widget_show(video_screen);
		sret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		status = 1;
		g_printerr("Called set pipeline to PLAYING. \n");
		if (sret == GST_STATE_CHANGE_FAILURE)
		{
			gst_element_set_state(pipeline, GST_STATE_NULL);
			g_printerr("Could not set pipeline to PLAYING. \n");
			status = 0;
			return -1;
		}
	}

	// And read the GUI also here, before the mutex to be unlocked:
	// gchar * text = gtk_entry_get_text(GTK_ENTRY(entry));
	g_mutex_unlock(&mutex_3);

	return FALSE;
}

void *start_loop_thread(void *arg)
{
	char buffer[5], img_filename[32]; 
	while (1)
	{
		// update time once a minute
		time(&current_time);
		time_info = localtime(&current_time);
		if (time_info->tm_min != current_min)
		{
			gdk_threads_add_idle(update_time, time_info);
			delay(1);
			current_min = time_info->tm_min;
		}
		switch (OpMode)
		{
			//{Splash = 0, Setup, Idle, BarGraph, PPM, LiveCam, IRCam, Shutdown}
		case Splash:
			gtk_widget_hide(ppm_display_label);
			gtk_widget_hide(eventbox_ppm);
			gtk_widget_hide(video_screen);
			gtk_widget_show(splash_screen);
			delay(3000);
			OpMode = Idle;
			gtk_widget_hide(splash_screen);
			break;
		case Setup:
			delay(1);
			break;

		case Idle:
			delay(1);
			gtk_widget_hide(gps_off);
			gtk_widget_hide(laser_off);
			gtk_widget_show(gps_on);
			gtk_widget_show(laser_on);
			if (status == 1)
			{
				/* Free the pipeline */
				sret = gst_element_set_state(pipeline, GST_STATE_PAUSED);

				if (sret == GST_STATE_CHANGE_FAILURE)
				{
					gst_element_set_state(pipeline, GST_STATE_NULL);
					g_printerr("Could not set pipeline to NULL. \n");
					status = 1;
				}
				else
				{
					status = 0;
					g_printerr("Called set pipeline to PAUSED. \n");
					// hide screen widget & show bkg
					gtk_widget_hide(video_screen);
					gtk_widget_show(eventbox_ppm);
					gtk_label_set_text(GTK_LABEL(left_label), (const gchar *)"Setup");
					gtk_label_set_text(GTK_LABEL(middle_label), (const gchar *)"Start");
					gtk_label_set_text(GTK_LABEL(right_label), (const gchar *)"Exit");
					gtk_label_set_text(GTK_LABEL(status_label), (const gchar *)"Idle");
				}
			}
			break;

		case PPM:
			mData.ppm += 1;
			mData.dist = UART_main();
			mData.ADVoltag = ADS1115_main();
			delay(1000);
			//gdk_threads_add_idle(update_ppm, (measData *)ppm);
			gdk_threads_add_idle(update_meas, (gpointer)&mData);
			break;
		case LiveCam:
			delay(1);
			gdk_threads_add_idle(live_stream, (int *)pipeline);
			break;
		case IRCam:
			delay(100);
			strcpy(img_filename, "./Images/Img_");
			{
				//char imgPath[64];
				//realpath(img_filename, imgPath);
				//printf("%s\n", imgPath);
			}
			if (counter < 10)
			{
				itoa((int)counter, buffer, 10);
				strcat(img_filename, buffer);
				strcat(img_filename, ".jpeg");

				g_object_get(sink, "last-sample", &from_sample, NULL);
				if (from_sample == NULL)
				{
					GST_ERROR("Error getting last sample form sink");
				}
				image_caps = gst_caps_from_string("image/jpeg");
				to_sample = gst_video_convert_sample(from_sample, image_caps, GST_CLOCK_TIME_NONE, &err);
				gst_caps_unref(image_caps);
				gst_sample_unref(from_sample);

				if (to_sample == NULL && err)
				{
					GST_ERROR("Error converting frame: %s", err->message);
					g_printerr("Error converting frame. \n");
					g_error_free(err);
				}
				buf = gst_sample_get_buffer(to_sample);
				if (gst_buffer_map(buf, &map_info, GST_MAP_READ))
				{
					if (!g_file_set_contents(img_filename, (const char *)map_info.data,
											 map_info.size, &err))
					{
						GST_CAT_LEVEL_LOG(GST_CAT_DEFAULT, GST_LEVEL_WARNING, NULL, "Could not save thumbnail: %s", err->message);
						g_error_free(err);
					}
				}
				counter++;
			}
			else
			{
				int cresult;
				counter = 0;
				//cresult = call_Python_Stitch(6, "Image_Stitching", "main", "Images", "output.jpeg","--images","--output");
				printf("The stitching is removed for test!\n");
				OpMode = Idle;
			}

			break;

		case Shutdown:
			delay(100);
			exit(0);
		}
	}
}

int main(int argc, char *argv[])
{

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGHUP, cleanup);

	// Intialize the wiringPi Library
	wiringPiSetupGpio();
	pinMode(LEFT_BUTTON, INPUT);
	pinMode(MIDDLE_BUTTON, INPUT);
	pinMode(RIGHT_BUTTON, INPUT);

	pullUpDnControl(LEFT_BUTTON, PUD_DOWN);
	pullUpDnControl(MIDDLE_BUTTON, PUD_DOWN);
	pullUpDnControl(RIGHT_BUTTON, PUD_DOWN);

	wiringPiISR(LEFT_BUTTON, INT_EDGE_FALLING, left_button_pressed);
	wiringPiISR(MIDDLE_BUTTON, INT_EDGE_FALLING, middle_button_pressed);
	wiringPiISR(RIGHT_BUTTON, INT_EDGE_FALLING, right_button_pressed);

	gtk_init(&argc, &argv); // init Gtk
	gst_init(&argc, &argv); // init Gstreamer

	/* Create the elements */
	source = gst_element_factory_make("libcamerasrc", "Source");
	filter = gst_element_factory_make("capsfilter", "filter");
	convert = gst_element_factory_make("videoconvert", "convert");
	sink = gst_element_factory_make("xvimagesink", "sink");
	videosrc_caps = gst_caps_new_simple("video/x-raw",
										"width", G_TYPE_INT, 480, "height", G_TYPE_INT, 480, NULL);
	/* Create the empty pipeline */
	flip = gst_element_factory_make("videoflip", "flip");
	g_object_set(G_OBJECT(flip), "method", 0, NULL);

	pipeline = gst_pipeline_new("test-pipeline");
	// pipeline = gst_parse_launch("appsrc libcamerasrc ! 'video/x-raw,width=480,height=480' ! videoconvert ! autovideosink", NULL);
	if (!pipeline || !source || !filter || !flip || !convert || !sink)
	{
		g_printerr("Not all elements could be created.\n");
		return -1;
	}

	/* Build the pipeline */
	gst_bin_add_many(GST_BIN(pipeline), source, filter, flip, convert, sink, NULL);
	if (gst_element_link_many(source, filter, flip, convert, sink, NULL) != TRUE)
	{
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	/* filter the pipeline*/
	g_object_set(G_OBJECT(filter), "caps", videosrc_caps, NULL);
	gst_caps_unref(videosrc_caps);

	builder = gtk_builder_new_from_file("gpioTest.glade");

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

	gtk_builder_connect_signals(builder, NULL);

	fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
	left_label = GTK_WIDGET(gtk_builder_get_object(builder, "left_label"));
	middle_label = GTK_WIDGET(gtk_builder_get_object(builder, "middle_label"));
	right_label = GTK_WIDGET(gtk_builder_get_object(builder, "right_label"));
	status_label = GTK_WIDGET(gtk_builder_get_object(builder, "status_label"));
	eventbox_label_l = GTK_WIDGET(gtk_builder_get_object(builder, "eventbox_label_l"));
	eventbox_label_m = GTK_WIDGET(gtk_builder_get_object(builder, "eventbox_label_m"));
	eventbox_label_r = GTK_WIDGET(gtk_builder_get_object(builder, "eventbox_label_r"));
	eventbox_status = GTK_WIDGET(gtk_builder_get_object(builder, "eventbox_status"));
	eventbox_ppm = GTK_WIDGET(gtk_builder_get_object(builder, "eventbox_ppm"));
	date_label = GTK_WIDGET(gtk_builder_get_object(builder, "date_label"));
	time_label = GTK_WIDGET(gtk_builder_get_object(builder, "time_label"));
	ppm_display_label = GTK_WIDGET(gtk_builder_get_object(builder, "ppm_display_label"));
	video_screen = GTK_WIDGET(gtk_builder_get_object(builder, "video_screen"));
	splash_screen = GTK_WIDGET(gtk_builder_get_object(builder, "splash_screen"));
	bat_label = GTK_WIDGET(gtk_builder_get_object(builder, "bat_label"));
	laser_on = GTK_WIDGET(gtk_builder_get_object(builder, "laser_on"));
	laser_off = GTK_WIDGET(gtk_builder_get_object(builder, "laser_off"));
	gps_on = GTK_WIDGET(gtk_builder_get_object(builder, "gps_on"));
	gps_off = GTK_WIDGET(gtk_builder_get_object(builder, "gps_off"));
	info_laser = GTK_WIDGET(gtk_builder_get_object(builder, "info_laser"));
	info_bat = GTK_WIDGET(gtk_builder_get_object(builder, "info_bat"));
	info_gps = GTK_WIDGET(gtk_builder_get_object(builder, "info_gps"));
	setup_fields_labels = GTK_WIDGET(gtk_builder_get_object(builder, "setup_fields_labels"));
	setup_fields_values = GTK_WIDGET(gtk_builder_get_object(builder, "setup_fields_values"));
	setup_label_1 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_1"));
	setup_label_2 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_2"));
	setup_label_3 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_3"));
	setup_label_4 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_4"));
	setup_label_5 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_5"));
	setup_label_6 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_6"));
	setup_label_7 = GTK_WIDGET(gtk_builder_get_object(builder, "setup_label_7"));
	setup_value_1 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_1"));
	setup_value_2 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_2"));
	setup_value_3 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_3"));
	setup_value_4 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_4"));
	setup_value_5 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_5"));
	setup_value_6 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_6"));
	setup_value_7 = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "setup_value_7"));

	GdkColor navy;
	navy.red = 0x1919;
	navy.green = 0x7474;
	navy.blue = 0xd2d2;

	GdkColor black;
	black.red = 0x0;
	black.green = 0x0;
	black.blue = 0x0;

	GdkColor white;
	white.red = 0xffff;
	white.green = 0xffff;
	white.blue = 0xffff;

	// res = gdk_rgba_parse (&color, "rgba(25, 116, 210,1)");
	gtk_widget_modify_bg(eventbox_label_l, GTK_STATE_NORMAL, &navy);
	gtk_widget_modify_bg(eventbox_label_m, GTK_STATE_NORMAL, &navy);
	gtk_widget_modify_bg(eventbox_label_r, GTK_STATE_NORMAL, &navy);
	gtk_widget_modify_bg(eventbox_status, GTK_STATE_NORMAL, &black);
	gtk_widget_modify_bg(eventbox_ppm, GTK_STATE_NORMAL, &white);
	gtk_widget_modify_bg(video_screen, GTK_STATE_NORMAL, &white);
	gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &white);
	gtk_widget_hide(gps_on);
	gtk_widget_hide(laser_on);
	gtk_widget_show(gps_off);
	gtk_widget_show(laser_off);

	OpMode = Splash;
	/* set the window position */
	gint x, y;
	x = 0; y = 1130;
	gtk_window_set_position(GTK_WINDOW(window), GDK_GRAVITY_NORTH_WEST);
	gtk_window_move(GTK_WINDOW(window), x, y);
	//gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	//gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	
	/* end the position set */
	gtk_widget_show(window);

	video_window_xwindow = gtk_widget_get_window(video_screen);
	embed_xid = GDK_WINDOW_XID(video_window_xwindow);
	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), embed_xid);

	/* threads id*/
	pthread_t tid;

	/* create threads */

	pthread_create(&tid, NULL, start_loop_thread, pipeline);
	/* wait for threads (warning, they should never terminate) */
	// pthread_join(tid, NULL);
	// setup_filestructure();

	gtk_main();

	return EXIT_SUCCESS;
}
