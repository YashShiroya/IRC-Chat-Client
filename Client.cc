
#include <stdio.h>
#include <gtk/gtk.h>
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)


GtkListStore * list_rooms;
GtkListStore * list_users;
GtkListStore * list_messages;
char * res;
GtkTreeSelection *gts;
GtkWidget *tree_view;
GtkWidget *messages;
GtkTextBuffer * gtb;
GtkTextBuffer *buffer_m;
GtkTreeIter iterr;
int check = 0;
static char buffer[256];

char *text_selected = (char*) g_malloc(sizeof(char) * 100);
char * room_selected = (char*) g_malloc(sizeof(char) * 100);
char * msg_room = (char*) g_malloc(sizeof(char) * 500);
char * msg_get =  (char*) g_malloc(sizeof(char) * 100);
gint login_check = 0;

typedef struct
{       
        GtkWidget *username;
        GtkWidget *password;
} userpass; 

//Networking____________________________________________

char * host = (char*) g_malloc(sizeof(char) * 100);
char * user = (char*) g_malloc(sizeof(char) * 100);
char * password = (char*) g_malloc(sizeof(char) * 100);
char * sport = (char*) g_malloc(sizeof(char) * 100);
int port;
//char * response = (char*)g_malloc(sizeof(char) * MAX_RESPONSE);





int lastMessage = 0;

int open_client_socket(char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}
//______________________________________________________

//Server Commands__________________________________________


int sendCommand(char * host, int port, char * command, char * user,
		char * password, char * args, char * response) {
	int sock = open_client_socket( host, port);

	// Send command
	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, user, strlen(user));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	write(sock, " ", 1);
	write(sock, args, strlen(args));
	write(sock, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}
	response[len - 1] = '\0';
	printf("response:%s\n", response);

	close(sock);
}


void printUsage()
{
	printf("Usage: talk-client host port user password\n");
	exit(1);
}

void add_user() {
	// Try first to add user in case it does not exist.
	char response[ MAX_RESPONSE ];
	response[0] = '\0';
	sendCommand(host, port, "ADD-USER", user, password, "", response);
	
	if (!strcmp(response,"OK\r\n")) {
		//printf("User %s added\n", user);
	}
}

void create_room(char * room_name) {
	// Try first to add user in case it does not exist.
	char response[ MAX_RESPONSE ];
	response[0] = '\0';
	sendCommand(host, port, "CREATE-ROOM", user, password, room_name, response);
	
	if (!strcmp(response,"OK\r\n")) {
		//printf("User %s added\n", user);
	}
}

void enter_room() {
	// Try first to add user in case it does not exist.
	char response[ MAX_RESPONSE ];
	response[0] = '\0';

	char * notification = (char*) g_malloc(sizeof(char) * 200);
	strcpy(notification,"");
	sprintf(notification,"User %s entered %s\n", user, room_selected);
	
	
	
	if(strcmp("default",room_selected) != 0) {
		sendCommand(host, port, "ENTER-ROOM", user, password, room_selected, response);
		sendCommand(host, port, "SEND-MESSAGE", user, password, notification, response);
	}
	if (!strcmp(response,"OK\r\n")) {
		//printf("User %s added\n", user);
	}
}

void leave_room() {
	// Try first to add user in case it does not exist.
	char response[ MAX_RESPONSE ];
	response[0] = '\0';
	
	char * notification = (char*) g_malloc(sizeof(char) * 200);
	strcpy(notification,"");
	sprintf(notification,"User %s left %s\n", user, room_selected);
	sendCommand(host, port, "SEND-MESSAGE", user, password, notification, response);
	
	if(strcmp("default",room_selected) != 0) {
		sendCommand(host, port, "LEAVE-ROOM", user, password, room_selected, response);
		sendCommand(host, port, "SEND-MESSAGE", user, password, notification, response);
	}
	if (!strcmp(response,"OK\r\n")) {
		//printf("User %s added\n", user);
	}
}

static void insert_text( GtkTextBuffer *buffer, const char * initialText )	////////////////////////////////// C R E A T E  T E X T ////////////////////////////////////
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;
   GtkWidget *view;
   

   view = gtk_text_view_new ();
   buffer_m = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		   	           GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

   gtk_container_add (GTK_CONTAINER (scrolled_window), view);
   insert_text (buffer_m, initialText);

   gtk_widget_show_all (scrolled_window);
   
   return scrolled_window;
}

void send_message(GtkWidget * message_entry) {
	char response[ MAX_RESPONSE ];
	response[0] = '\0';
	strcpy(msg_room,"");
	
	char * sent_message = strdup(gtk_entry_get_text(GTK_ENTRY(message_entry)));
	if(strcmp("default",room_selected) != 0) {												//Could cause errors.......................................................
		
		strcat(msg_room, room_selected);
		strcat(msg_room, " ");
		strcat(msg_room, sent_message);
		
		sendCommand(host, port, "SEND-MESSAGE", user, password, msg_room, response);
	}
	else {
		printf("Client Request, select a room\n");
	}
	if (!strcmp(response,"OK\r\n")) {
		//printf("User %s added\n", user);
	}
}


#define MAXWORD 200
 
  

//________________________________________________________________________________________________________
void update_list_rooms() {
		
		int wordLength = 0;
		char word[MAXWORD];
    
    //nt i = 0;
		char response[ MAX_RESPONSE ];
		sendCommand(host, port, "LIST-ROOMS", user, password, "", response);
		
		char * t; char * res;
		//nextword
		res = strdup(response);
		int c; int i = 0;
	
	while((c = *res) != '\0') {
	    if(c != '\n' && c != '\r') {
	        word[i++] = c;
	    }
	    else if(c == '\r') {
	    	res++;
	        if(i > 0) {
	        	word[i] = '\0';
	        	i = 0;
				t = strdup(word);
				gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iterr);
        		gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iterr, 0, t, -1);
				
			}
	   }
	   //printf("c %c\n",c);
	   res++;
	}

    
}

void update_list_users() {

    
    //nt i = 0;
    	
    	int wordLength = 0;
		char word[MAXWORD];
    	GtkTreeIter iter;
    		
		char response[ MAX_RESPONSE ];
		sendCommand(host, port, "GET-USERS-IN-ROOM", user, password, room_selected, response);
		
		char * t; char * res;
		//nextword
		res = strdup(response);
		int c; int i = 0;
	
	while((c = *res) != '\0') {
	    if(c != '\n' && c != '\r') {
	        word[i++] = c;
	    }
	    else if(c == '\r') {
	    	res++;
	        if(i > 0) {
	        	word[i] = '\0';
	        	i = 0;
				t = strdup(word);
				gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
        		gtk_list_store_set (GTK_LIST_STORE (list_users), &iter, 0, t, -1);
				
			}
	   }
	   //printf("c %c\n",c);
	   res++;
	}

    
}

void update_list_messages() {

    
    //nt i = 0;
    	
    	int wordLength = 0;
		char word[MAXWORD];
    	GtkTreeIter iter;
    		
		char response[ MAX_RESPONSE ];
		response[0] = '\0';
		strcpy(msg_get,"");
		strcat(msg_get,"0 ");
	
	//gtb = gtk_text_view_get_buffer (GTK_TEXT_VIEW(messages));
	
	if(strcmp("default",room_selected) != 0) {
		strcat(msg_get, room_selected);
		sendCommand(host, port, "GET-MESSAGES", user, password, msg_get, response);
		
		
	}		
		
		char * t; char * res;
		//nextword
		res = strdup(response);
		int c; int i = 0;
	
	while((c = *res) != '\0') {
	    if(c != '\n' && c != '\r') {
	        word[i++] = c;
	    }
	    else if(c == '\r') {
	    	res++;
	        if(i > 0) {
	        	word[i] = '\0';
	        	i = 0;
				t = strdup(word);
				gtk_list_store_append (GTK_LIST_STORE (list_messages), &iter);
        		gtk_list_store_set (GTK_LIST_STORE (list_messages), &iter, 0, t, -1);
				
			}
	   }
	   //printf("c %c\n",c);
	   res++;
	}

    
}
static gboolean time_handler(GtkWidget *widget)
{
  if (widget->window == NULL) return FALSE;
  if(check == 1) {	
	gtk_list_store_clear (list_rooms);
	gtk_list_store_clear (list_messages);
	//gtk_list_store_clear (list_users);
	update_list_rooms();
	
	update_list_messages();
  }	
  	return TRUE;
}

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
	  		         GTK_TREE_VIEW_COLUMN (column));
	//g_signal_connect(tree_view, "row-activated", G_CALLBACK(update_list_rooms), NULL);
		
    return scrolled_window;
}
   
static GtkWidget * create_list_users( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    GtkWidget *tree_view_r;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view_r = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view_r);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view_r), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view_r);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view_r),
	  		         GTK_TREE_VIEW_COLUMN (column));
	//g_signal_connect(tree_view, "row-activated", G_CALLBACK(update_list_rooms), NULL);
		
    return scrolled_window;
}   

static GtkWidget * create_list_messages( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    GtkWidget *tree_view_r;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view_r = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view_r);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view_r), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view_r);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view_r),
	  		         GTK_TREE_VIEW_COLUMN (column));
	//g_signal_connect(tree_view, "row-activated", G_CALLBACK(update_list_rooms), NULL);
		
    return scrolled_window;
}
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */


//______________________________________________________________________________________________________________


static void login_callback(userpass * up) {

	
	user = strdup(gtk_entry_get_text(GTK_ENTRY(up->username)));
	password = strdup(gtk_entry_get_text(GTK_ENTRY(up->password)));
	check = 1;
	add_user();
}

static void create_room_callback(GtkWidget * entry) {

	char * room_name = (char*) g_malloc(sizeof(char) * 100);
	//user = strdup(gtk_entry_get_text(GTK_ENTRY(up->username)));
	//password = strdup(gtk_entry_get_text(GTK_ENTRY(up->password)));
	room_name = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	create_room(room_name);
}

static void listrooms_callback() {
		printf("lr callback\n");
		gtk_list_store_clear (list_rooms);
		update_list_rooms();
		
	
}



//static void listusers_callback() {
//		printf("lr callback\n");
		
	//	update_list_users();
		
	
//}



/*static void enter_callback( GtkWidget *widget,
                            GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  printf ("Entry contents: %s\n", entry_text);
}*/

static void entry_toggle_editable( GtkWidget *checkbutton,
                                   
                                   GtkWidget *entry )
{
  gtk_editable_set_editable (GTK_EDITABLE (entry),
                             GTK_TOGGLE_BUTTON (checkbutton)->active);
}

static void gtk_themer_blue(GtkWidget *widget) {
	GdkColor color;
	gint col = 65535/225; 
    color.red = col * 90;
    color.blue = col * 400;
    color.green = col * 120;
    
    gtk_widget_modify_bg (GTK_WIDGET(widget), GTK_STATE_NORMAL, &color);
}


static void gtk_themer_dark(GtkWidget *widget) {
	GdkColor color;
	gint col = 65535/225; 
    color.red = col * 20;
    color.blue = col * 70;
    color.green = col * 100;
    
    gtk_widget_modify_bg (GTK_WIDGET(widget), GTK_STATE_NORMAL, &color);
}





static void tree_changed(GtkWidget * widget) {
	
	printf("Callback tree_changed\n");
	gtk_list_store_clear (list_users);
	//GtkTreeModel *model;
	text_selected = "default\n";
  GtkTreeIter iter;
  GtkTreeModel *model;
 


  if (gtk_tree_selection_get_selected(
      GTK_TREE_SELECTION(widget), &model, &iter)) {

    gtk_tree_model_get(model, &iter, 0, &text_selected,  -1);
    
    
  }
  	room_selected = strdup(text_selected);
  	update_list_users();
  	printf("selected %s\n",text_selected);
	g_free(text_selected);
}

static void gtk_themer_crimson(GtkWidget *widget) {
	GdkColor color;
	gint col = 65535/225; 
    color.red = col * 450;
    color.blue = col * 450;
    color.green = col * 450;
    
    gtk_widget_modify_bg (GTK_WIDGET(widget), GTK_STATE_NORMAL, &color);
}
//______________________________________________________________

int main( int   argc,
          char *argv[] )
{
    strcpy(host,"localhost");
    port = 2408;
    GtkWidget *window;
    GtkWidget *list_r;
    GtkWidget *list_u;
    GtkWidget *list_m;
    
    GtkWidget *myMessage;
    userpass * userInfo;
    
    userInfo = g_slice_new(userpass);
    
    
    //LOGIN
     GtkWidget *vbox, *hbox;
    //GtkWidget *entry;
    //GtkWidget *entry_u;
    GtkWidget *button;
    GtkWidget *button_sign_up;
    GtkWidget *button_sign_in;
    GtkWidget *check;
    GtkWidget *login_frame;
    GtkWidget *separator;
    GtkWidget *theme_frame;
    GtkWidget *theme_blue;
    GtkWidget *theme_crimson;
    GtkWidget *theme_dark;
    GtkWidget *v_bbox;
    GtkWidget *v_bbox2;
    GtkWidget *frame_door;
    GtkWidget *enter_b;
    GtkWidget *leave_b;
    GtkWidget *create_b;
    GtkWidget *v_bbox3;
    GtkWidget *msg_frame;
    GtkWidget *v_box_msg;
    GtkWidget *create_e;
    GtkWidget *listrooms_b;
    GtkWidget *msg_entry;
    
    //GTK Pointer
    
    userInfo->username = gtk_entry_new();
    userInfo->password = gtk_entry_new();
    
   	//userpass[1] = entry_u;
    gint tmp_pos;
    //

    gtk_init (&argc, &argv);
   
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Paned Windows");
    g_signal_connect (window, "destroy",
	              G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 1000, 700);

    // Create a table to place the widgets. Use a 7x4 Grid (7 rows x 4 columns)
    GtkWidget *table = gtk_table_new (12, 14, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 20);
    gtk_table_set_col_spacings(GTK_TABLE (table), 20);
    gtk_widget_show (table);

  //____________________________LOGIN____________________________________________

	login_frame = gtk_frame_new("Login");
   	//gtk_container_add(GTK_CONTAINER (window), login_frame);
   	gtk_widget_show(login_frame);
   	
   	vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (login_frame), vbox);
    gtk_widget_show (vbox);
   
    
    //User field
    userInfo->username = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (userInfo->username), 50);
    //const gchar * text = "Username";
    //gtk_entry_set_placeholder_text (GTK_ENTRY(userInfo->username),text);
    gtk_entry_set_text(GTK_ENTRY(userInfo->username), "Username");
    gtk_box_pack_start (GTK_BOX (vbox), userInfo->username, TRUE, TRUE, 0);
    gtk_widget_show(userInfo->username);
	
	//____    
    
    //Password field
    userInfo->password = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (userInfo->password), 50);
    //const gchar * text2 = "Password";
    //gtk_entry_set_placeholder_text (GTK_ENTRY(userInfo->password),text2); 
    gtk_entry_set_text(GTK_ENTRY(userInfo->password), "*********");  
    gtk_entry_set_visibility (GTK_ENTRY (userInfo->password), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), userInfo->password, TRUE, TRUE, 0);
    gtk_widget_show (userInfo->password);


    //________Added                               
    
                                     
    button_sign_up = gtk_button_new_with_label ("Sign Up");
    g_signal_connect_swapped (button_sign_up, "clicked",
			      G_CALLBACK (gtk_widget_destroy),										//Change this callback
			      window);													
    gtk_box_pack_start (GTK_BOX (vbox), button_sign_up, TRUE, TRUE, 0);
    gtk_widget_set_can_default (button_sign_up, TRUE);
    gtk_widget_grab_default (button_sign_up);
    gtk_widget_show (button_sign_up);
     
    button_sign_in = gtk_button_new_with_label ("Sign In");
    g_signal_connect_swapped (button_sign_in, "clicked",
			      G_CALLBACK (login_callback),										//Change this callback
			      userInfo);
			      
														
    gtk_box_pack_start (GTK_BOX (vbox), button_sign_in, TRUE, TRUE, 0);
    gtk_widget_set_can_default (button_sign_in, TRUE);
    gtk_widget_grab_default (button_sign_in);
    gtk_widget_show (button_sign_in);
    
    gtk_table_attach_defaults(GTK_TABLE (table), login_frame, 0, 4, 0, 5); 
    
    

//LOGIN END_________________________________________________________________________________________________________________

    
    
    
    
    
    // Add list of rooms. Use columns 0 to 4 (exclusive) and rows 0 to 4 (exclusive)
    if(login_check == 0) {
    
    /////////////////////////////// R       O       O        M        S/////////////////////////////////////////
    
    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    //update_list_rooms();
    
    list_r = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list_r, 10, 14, 0, 4);
    
    //g_signal_connect(gts, "changed", 
      //G_CALLBACK(tree_changed), NULL);
      gts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
      g_signal_connect_swapped(tree_view, "row-activated", G_CALLBACK(tree_changed), gts);
      
    gtk_widget_show (list_r);
    
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////user tree removed
    //List of users
    list_users = gtk_list_store_new (1, G_TYPE_STRING);
    list_u = create_list_users ("Users In Room", list_users);
    gtk_table_attach_defaults (GTK_TABLE (table), list_u, 0, 4, 5, 12);
    gtk_widget_show (list_u);
    
   
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    list_messages = gtk_list_store_new (1, G_TYPE_STRING);
    list_m = create_list ("Messages", list_messages);
    gtk_table_attach_defaults (GTK_TABLE (table), list_m, 4, 10, 0, 7);
    gtk_widget_show (list_m);

   
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 

	msg_frame = gtk_frame_new("Write Message");
    gtk_widget_show(msg_frame); 
    
    v_box_msg = gtk_vbox_new(FALSE,0);
    gtk_container_add (GTK_CONTAINER (msg_frame), v_box_msg);
    gtk_widget_show (v_box_msg);
	

    //myMessage = create_text ("I am fine, thanks and you?\n");
    msg_entry = gtk_entry_new();
      	
   gtk_box_pack_start (GTK_BOX (v_box_msg), msg_entry, TRUE, TRUE, 0);
   gtk_widget_set_size_request (GTK_WIDGET (msg_entry), 100, 200);
    gtk_widget_show (msg_entry);

    // Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    //gtk_table_attach_defaults(GTK_TABLE (table), send_button, 4, 10, 11, 12);
    gtk_box_pack_start (GTK_BOX (v_box_msg), send_button, TRUE, TRUE, 0);
    g_signal_connect_swapped (send_button, "clicked",
			      G_CALLBACK (send_message),										//Change this callback
			      msg_entry);
    gtk_widget_show (send_button);
   
   	
   
   
    
    //THEME________________________________________________________
    
    theme_frame = gtk_frame_new("Themes");
    gtk_widget_show(theme_frame); 
    
    v_bbox = gtk_vbutton_box_new();
    gtk_container_add (GTK_CONTAINER (theme_frame), v_bbox);
    gtk_widget_show (v_bbox);

    
    theme_blue = gtk_button_new_with_label ("Clouds");
 	gtk_box_pack_start (GTK_BOX (v_bbox), theme_blue, TRUE, TRUE, 0);	
    g_signal_connect_swapped (theme_blue, "clicked",
			      G_CALLBACK (gtk_themer_blue),										//Change this callback
			      window);
	gtk_widget_set_can_default (theme_blue, TRUE);
    gtk_widget_grab_default (theme_blue);
    			
	gtk_widget_show(theme_blue);
	
	theme_dark = gtk_button_new_with_label ("Grasslands");
 	gtk_box_pack_start (GTK_BOX (v_bbox), theme_dark, TRUE, TRUE, 0);	
    g_signal_connect_swapped (theme_dark, "clicked",
			      G_CALLBACK (gtk_themer_dark),										//Change this callback
			      window);
	gtk_widget_set_can_default (theme_dark, TRUE);
    gtk_widget_grab_default (theme_dark);
    			
	gtk_widget_show(theme_dark);
	
	theme_crimson = gtk_button_new_with_label ("Serenity");
 	gtk_box_pack_start (GTK_BOX (v_bbox), theme_crimson, TRUE, TRUE, 0);	
    g_signal_connect_swapped (theme_crimson, "clicked",
			      G_CALLBACK (gtk_themer_crimson),										//Change this callback
			      window);
	gtk_widget_set_can_default (theme_crimson, TRUE);
    gtk_widget_grab_default (theme_crimson);
    			
	gtk_widget_show(theme_crimson);

	
	//________________________Theme/____________________________	      									
    
    
    //_______
    
    
    //__________________Create Room______________
    
    v_bbox3 = gtk_vbutton_box_new();
    gtk_table_attach_defaults(GTK_TABLE (table), v_bbox3, 10, 14, 4, 6);
    gtk_widget_show (v_bbox3);
    
    create_e = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (create_e), 50);
    gtk_entry_set_text (GTK_ENTRY (create_e), "Enter Room Name here.");
    gtk_box_pack_start (GTK_BOX (v_bbox3), create_e, TRUE, TRUE, 0);
    gtk_widget_show(create_e);
    
    
    create_b = gtk_button_new_with_label ("Create Room");
    g_signal_connect_swapped (create_b, "clicked",
			      G_CALLBACK (create_room_callback),										//Change this callback
			      create_e);													
  	gtk_box_pack_start (GTK_BOX (v_bbox3), create_b, TRUE, TRUE, 0);
    gtk_widget_set_can_default (create_b, TRUE);
    gtk_widget_grab_default (create_b);
    gtk_widget_set_size_request (GTK_WIDGET (create_b), 250, 40);    gtk_widget_show (create_b);
    
    //____________________________________________
        
   //___________Door__________________
    
    frame_door = gtk_frame_new("Door");
    gtk_widget_show(frame_door);
    
    v_bbox2 = gtk_vbutton_box_new();
    gtk_container_add (GTK_CONTAINER (frame_door), v_bbox2);
    gtk_widget_show (v_bbox2);
    
    enter_b = gtk_button_new_with_label ("Enter Room");
    g_signal_connect_swapped (enter_b, "clicked",
			      G_CALLBACK (enter_room),										//Change this callback
			      NULL);													
    gtk_box_pack_start (GTK_BOX (v_bbox2), enter_b, TRUE, TRUE, 0);
    gtk_widget_set_can_default (enter_b, TRUE);
    gtk_widget_grab_default (enter_b);
    gtk_widget_set_size_request (GTK_WIDGET (enter_b), 250, 40);
    gtk_widget_show (enter_b);
    
    leave_b = gtk_button_new_with_label ("Leave Room");
    g_signal_connect_swapped (leave_b, "clicked",
			      G_CALLBACK (leave_room),										//Change this callback
			      NULL);													
    gtk_box_pack_start (GTK_BOX (v_bbox2), leave_b, TRUE, TRUE, 0);
    gtk_widget_set_can_default (leave_b, TRUE);
    gtk_widget_grab_default (leave_b);
    gtk_widget_set_size_request (GTK_WIDGET (leave_b), 250, 40);
    gtk_widget_show (leave_b);
    
    /*listrooms_b = gtk_button_new_with_label ("List Rooms");
    g_signal_connect (listrooms_b, "clicked",
			      G_CALLBACK (listrooms_callback),										//Change this callback
			      NULL);													
    gtk_box_pack_start (GTK_BOX (v_bbox2), listrooms_b, TRUE, TRUE, 0);
    gtk_widget_set_can_default (listrooms_b, TRUE);
    gtk_widget_grab_default (listrooms_b);
    gtk_widget_set_size_request (GTK_WIDGET (listrooms_b), 250, 40);
    gtk_widget_show (listrooms_b);*/
    
   //______________________________
   
    	//____GGGGGGG

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox);
    gtk_widget_show (hbox);
                                                                    
    button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (gtk_widget_destroy),
			      window);
    gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
    gtk_widget_set_can_default (button, TRUE);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    
    
    //__________GGGGGG
    
    gtk_table_attach_defaults(GTK_TABLE (table), theme_frame, 12, 14, 10, 12);
    gtk_table_attach_defaults(GTK_TABLE (table), frame_door, 10, 14, 6, 8);
    gtk_table_attach_defaults (GTK_TABLE (table), msg_frame, 4, 10, 7, 12);
     
    GtkWidget *view;
	GtkTextBuffer *buffer;
	view = gtk_text_view_new ();

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_set_text (buffer, "© Yash Shiroya", -1);
	gtk_table_attach_defaults(GTK_TABLE (table), view, 10, 12, 11, 12);
	gtk_widget_set_size_request (GTK_WIDGET (view), 100, 20);
	
	gtk_widget_show(view);
    
    //______________________________________________________________________________
   } 
    
    //gdk_color_parse("red_float", &color);
    g_timeout_add(2000, (GSourceFunc) time_handler, (gpointer) window);
    gtk_widget_show (table);
    gtk_widget_show (window);
    time_handler(window);
    

    gtk_main ();

    return 0;
}


