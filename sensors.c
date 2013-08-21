#include "sensors.h"


static int		ncpu;
static GtkWidget 	*ld[MAXNCPU][6]; 	/* array of gtk_labels with refreshing data */
static long		cp_last[MAXNCPU*CP_STATES]; /* saved kern.cp_times values */

static gboolean		window_hidden; 		/* main window status */


typedef struct s_usage {
		float	user;
		float	sys;
		float	intr;
		float 	idle;
		float	total;
	} usage_t;		/* struct to hold cpu usage returned by sysctl */






static int
get_num_cpu(void)
{
	int n;
	size_t len;
	
	len = sizeof(n);

	if(sysctlbyname(NCPU_MIB, &n, &len, NULL, 0) == -1 ){
		perror("get_num_cpu() sysctl");
		return -1;
	}else{

		if(n > 1)
			printf("%d CPUs detected\n", n);
		else
			printf("%d CPU detected\n", n);
		return n;
	}
}





static int
get_freq(const int cpu)
{
	int 	freq;
	size_t 	len;

	len = sizeof(freq);

	if(sysctlbyname(FREQ_MIB, &freq, &len, NULL, 0) == -1 ){
		perror("get_freq() sysctl");
		return -1;
	}else{
		return freq;
	}
}



static float
get_temp(const int cpu)
{
	size_t len;
	int	t = 0;
	float	c = 0.0f;
	char	ACPI_TEMP_MIB[35];
	char	CORE_TEMP_MIB[26];
	len = sizeof(t);

	snprintf(ACPI_TEMP_MIB, sizeof(ACPI_TEMP_MIB), "hw.acpi.thermal.tz%d.temperature", cpu);
	snprintf(CORE_TEMP_MIB, sizeof(CORE_TEMP_MIB), "dev.cpu.%d.temperature", cpu);

	if((sysctlbyname(ACPI_TEMP_MIB, &t, &len, NULL, 0) == -1 ) && (sysctlbyname(CORE_TEMP_MIB, &t, &len, NULL, 0) == -1)){
		printf("ERROR: can not read CPU temperature, (get_temp(): %s)\n\t try to kldload coretemp.ko and restart program\n\n", strerror(errno));
		return -1.0f;
	}else{
		c = (t/10-273.15f); /* convert to Celsius */
		return (c);
	}
}




static int
get_cpu_usage(usage_t *usage, const int cpu)
{
	register int	offset,state;
	register long	summ, tmp;
	size_t 		cp_sz;
	long 		cp_current[CP_STATES*ncpu];


	cp_sz	= sizeof(cp_current);
	bzero(&cp_current, cp_sz);


	if (sysctlbyname(CP_TIMES_MIB, &cp_current, &cp_sz, NULL, 0) < 0){
		printf ("ERROR: get_cpu_usage() sysctl\n");
		return -1;
	}

	offset = cpu * CP_STATES; /* offset in array for selected cpu */

	summ = 0;

	for (state = 0; state < CP_STATES; state++){ /* get deltas and summ them up */

		tmp = cp_current[state+offset];
		cp_current[state+offset] -= cp_last[state+offset];
		cp_last[state+offset] = tmp;
		summ += cp_current[state+offset];
	}


	/* calculate percentages */
	(*usage).user = 100.0f - (100.0f - (100.0f * cp_current[CP_USER+offset]/(summ ? (float)summ : 1.0f))); 
	(*usage).sys = 100.0f - (100.0f - (100.0f * cp_current[CP_SYS+offset]/(summ ? (float)summ : 1.0f))); 
	(*usage).total = 100.0f - (100.0f * cp_current[CP_IDLE+offset]/(summ ? (float)summ : 1.0f)); 
	
	return 0;
}
















static gboolean
update_freq(const int *cpu)
{
	int 		freq;
	char 		sbuf[15];
	GtkWidget	*widget;

	widget = ld[*cpu][FREQ]; /* we always take frequency value from cpu0*/

	if ((freq=get_freq(*cpu)) > 0){
		
		snprintf(sbuf, sizeof(sbuf), "%d MHz", freq);
		gtk_label_set_markup(GTK_LABEL(widget), sbuf);
		return TRUE;
	}else{
		snprintf(sbuf, sizeof(sbuf), ERRLABEL);
		return FALSE; /* stop updating freq */
	}

}



static gboolean
update_temp(const int *cpu)
{
	float 		temp;
	char		sbuf[15];
	GtkWidget	*widget;

	widget = ld[*cpu][TEMP];

	if ((temp = get_temp(*cpu)) > 0){
		snprintf(sbuf, sizeof(sbuf), "%.1f C", temp);
		gtk_label_set_markup(GTK_LABEL(widget), sbuf);
  		return TRUE;
	}else{
		snprintf(sbuf, sizeof(sbuf), ERRLABEL);
		gtk_label_set_markup(GTK_LABEL(widget), sbuf);
		return FALSE; /* stop updating temp */
	}

}






static gboolean
update_cpu_usage(const int *cpu)
{
	char 		sbuf[15];
	GtkWidget	*widget1;
	GtkWidget	*widget2;
	GtkWidget	*widget3;
	usage_t		usage;

	
	widget1 = ld[*cpu][USER];
	widget2 = ld[*cpu][SYS];
	widget3 = ld[*cpu][TOTAL];


	if (get_cpu_usage(&usage, *cpu) < 0.0L ){

		g_print("update_cpu_usage() error\n");
		snprintf(sbuf, sizeof(sbuf), ERRLABEL);
		gtk_label_set_markup(GTK_LABEL(widget1), sbuf);
		gtk_label_set_markup(GTK_LABEL(widget2), sbuf);
		gtk_label_set_markup(GTK_LABEL(widget3), sbuf);
		return FALSE;  /* stop updating */
	}


	snprintf(sbuf, sizeof(sbuf), "%0.2f%%", usage.user);
	gtk_label_set_markup(GTK_LABEL(widget1), sbuf);

	bzero(sbuf, sizeof(sbuf));

	snprintf(sbuf, sizeof(sbuf), "%0.2f%%", usage.sys);
	gtk_label_set_markup(GTK_LABEL(widget2), sbuf);

	bzero(sbuf, sizeof(sbuf));

	snprintf(sbuf, sizeof(sbuf), "%0.2f%%", usage.total);
	gtk_label_set_markup(GTK_LABEL(widget3), sbuf);

	return TRUE;
}








static void
do_drawing(cairo_t *cr)
{
	int 	freq;
	char 	strfreq[15];

	if ((freq=get_freq(1)) <= 0){
		g_print("freq get error\n");
		return;
	}

	snprintf(strfreq, sizeof(strfreq), "Freq: %d", freq);
	
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_rectangle(cr, 10,10,250,60);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 255,255,255);
	cairo_select_font_face(cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 30.0);

	cairo_move_to(cr, 10.0, 40.0);
	cairo_show_text(cr, strfreq);

}













/*		EVENTS			*/


static gboolean
on_draw_event(GtkWidget *widget )
{
	cairo_t *cr;

	cr = gdk_cairo_create(gtk_widget_get_window(widget));
	do_drawing(cr);
	cairo_destroy(cr);

	return TRUE;
}



static gboolean
status_icon_click(GObject *icon, gpointer window)
{

	if ( window_hidden ){
		gtk_window_deiconify(GTK_WINDOW(window));
	}else{
		gtk_window_iconify(GTK_WINDOW(window));
	}


	return TRUE;
}





static gboolean
window_state_event(GtkWidget *window, GdkEventWindowState *event, gpointer icon)
{
	if (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED && (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED))){
		/* gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE); */
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
		g_print("hide \n");
		window_hidden = TRUE;

	}else if (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED && (event->new_window_state != GDK_WINDOW_STATE_ICONIFIED )){
		
		/* gtk_window_set_skip_pager_hint(GTK_WINDOW(window), FALSE); */
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), FALSE);
		g_print("show\n");
		window_hidden = FALSE;
	}

	return TRUE;

}





static gboolean
delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_print("detele event\n");
	return FALSE;
}



static void
destroy (GtkWidget *widget, gpointer data)
{
	g_print("exit\n");
	gtk_main_quit();
}







/*		MAIN		*/


int main (int argc, char *argv[])
{

	int	refresh = 0;
	int	i; 
	char 	label[16]; /* cpu name holder */
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *button2;
	GtkWidget *darea;
	GtkWidget *table;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkStatusIcon *icon;



	if ((ncpu = get_num_cpu()) < 0){ /* get a number of cpus in a system */
		fprintf(stderr,"Cannot count ncpu\n");
		exit(1);
	}
	
	int 	j[ncpu]; 	/* cpu numbers for looping watchdogs */
	gint 	wd_freq[ncpu]; 	/* watchdog object  arrays */
	gint 	wd_temp[ncpu];
	gint 	wd_cpu_usage[ncpu];


	for (i=0; i < (ncpu * CP_STATES); i++){ /* initialize saved cpu states array */
		cp_last[i] = 0; 		/* saved states for cpu usage */
	}



	
	gtk_init(&argc, &argv);



	/* WINDOW */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);

	gtk_window_set_resizable(GTK_WINDOW(window), 0);

	gtk_window_set_title(GTK_WINDOW(window), PROGNAME);
	gtk_window_set_default_size(GTK_WINDOW(window), 430, 250);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), FALSE);

	gtk_container_set_border_width(GTK_CONTAINER(window), 5);



	/* TABLE */
	table = gtk_table_new(ncpu+1, 8, FALSE);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

		/* TABLE HEADER */
		label1 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label1), L1);
		gtk_table_attach_defaults(GTK_TABLE(table),label1, 0,1,0,1);

		label2 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label2), L2);
		gtk_table_attach_defaults(GTK_TABLE(table),label2, 1,2,0,1);

		label3 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label3), L3);
		gtk_table_attach_defaults(GTK_TABLE(table),label3, 2,3,0,1);

		label4 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label4), L4);
		gtk_table_attach_defaults(GTK_TABLE(table),label4, 3,4,0,1);

		label5 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label5), L5);
		gtk_table_attach_defaults(GTK_TABLE(table),label5, 5,6,0,1);

		label6 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label6), L6);
		gtk_table_attach_defaults(GTK_TABLE(table),label6, 7,8,0,1);

		/* LABELS */
		for (i = 0; i != ncpu; i++){


			snprintf(label, sizeof(label), "<b>%d:</b>", i );

			ld[i][0] = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(ld[i][0]), label);
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][0], 0,1,i+1,i+2);

			ld[i][FREQ] = gtk_label_new(NULL); /* FREQ */
			gtk_label_set_markup(GTK_LABEL(ld[i][FREQ]), "0");
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][FREQ], 1,2,i+1,i+2);

			ld[i][TEMP] = gtk_label_new(NULL); /* TEMP */
			gtk_label_set_markup(GTK_LABEL(ld[i][TEMP]), "0");
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][TEMP], 2,3,i+1,i+2);

			ld[i][USER] = gtk_label_new(NULL); /* USER */
			gtk_label_set_markup(GTK_LABEL(ld[i][USER]), "0");
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][USER], 3,4,i+1,i+2);

			ld[i][SYS] = gtk_label_new(NULL); /* SYS */
			gtk_label_set_markup(GTK_LABEL(ld[i][SYS]), "0");
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][SYS], 5,6,i+1,i+2);

			ld[i][TOTAL] = gtk_label_new(NULL); /* TOTAL */
			gtk_label_set_markup(GTK_LABEL(ld[i][TOTAL]), "0");
			gtk_table_attach_defaults(GTK_TABLE(table), ld[i][TOTAL], 7,8,i+1,i+2);
		}


		/* BUTTONS */
		/*button = gtk_button_new_with_label ("Draw");
		gtk_widget_set_size_request(button, 70, 30);*/
		button2 = gtk_button_new_with_label ("Hide");
		gtk_widget_set_size_request(button2, 70, 30);
		/* g_signal_connect(button, "clicked", G_CALLBACK(on_draw_event), NULL); */
		g_signal_connect(button2, "clicked",GTK_SIGNAL_FUNC(status_icon_click), G_OBJECT(window));
		/* gtk_table_attach_defaults(GTK_TABLE(table),button, 5, 6, ncpu+1, ncpu+2); */
		gtk_table_attach_defaults(GTK_TABLE(table),button2, 7, 8, ncpu+1, ncpu+2);


	gtk_container_add(GTK_CONTAINER(window), table);



	/* DRAWAREA */
	/*darea = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), darea, TRUE, TRUE, 0);	*/

	/*g_signal_connect(G_OBJECT(darea), "expose-event", G_CALLBACK(on_draw_event), NULL);*/







	/* ICON */
	icon = gtk_status_icon_new_from_stock(GTK_STOCK_OPEN);
	gtk_status_icon_set_tooltip(GTK_STATUS_ICON(icon), PROGNAME);

	g_signal_connect(GTK_STATUS_ICON(icon), "activate", GTK_SIGNAL_FUNC(status_icon_click), G_OBJECT(window));
	g_signal_connect(G_OBJECT(window), "window-state-event", G_CALLBACK(window_state_event), G_OBJECT(icon));





	/* UPDATE */

		if ( argc > 1 ){
			refresh = (int)strtol(argv[1], (char **)NULL, 10);
		}

	refresh = (refresh <= 0 || refresh > MAXREFRESH)? REFRESH : refresh;

	printf("refresh every %d seconds\n", refresh);

	 /* we need to pass a pointer to a variable containing cpu number in ld[] array, for updating widgets */
	for (i=0; i != ncpu; i++){ /* set watchdogs for each cpu */
		
		j[i] = i;
		wd_freq[i] = g_timeout_add_seconds(refresh, (GSourceFunc)update_freq, &j[i]);
		wd_temp[i] = g_timeout_add_seconds(refresh, (GSourceFunc)update_temp, &j[i]);
		wd_cpu_usage[i] = g_timeout_add_seconds(refresh, (GSourceFunc)update_cpu_usage, &j[i]);
	}





	gtk_widget_show_all(window);


	gtk_main();


	return 0;



}


