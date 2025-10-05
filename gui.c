#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>

// include iceprog library
#include "iceprog_fn.h"
#include "mpsse.h"

static bool mpsse_initialized = false;

static void cleanup_mpsse() {
    if (mpsse_initialized) {
        printf("Cleaning up MPSSE interface...\n");
        mpsse_close();
        mpsse_initialized = false;
    }
}

static void on_window_destroy(GtkWidget *widget, gpointer data) {
    cleanup_mpsse();
    gtk_main_quit();
}

static void on_clicked(GtkButton *button, gpointer user_data) {
    printf("Button clicked!\n");
}

static void on_btn_test_connection(GtkButton *button, gpointer user_data) {
    printf("Testing SPI Flash connection...\n");
    
    // Initialize MPSSE if not already done
    if (!mpsse_initialized) {
        printf("Initializing MPSSE interface...\n");
        
        // Use default parameters: interface 0, no device string, normal clock speed
        mpsse_init(0, NULL, false);
        mpsse_initialized = true;
        
        printf("MPSSE initialized successfully\n");
        
        // Release reset and setup flash
        flash_release_reset();
        usleep(100000);
        
        printf("Flash reset released\n");
    }
    
    // Test the flash connection
    printf("Reading flash ID...\n");
    flash_reset();
    flash_power_up();
    flash_read_id();
    flash_power_down();
    printf("Flash test completed\n");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IceProg GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Add a button to test SPI flash connection
    GtkWidget *btn_test_connection = gtk_button_new_with_label("Test SPI Flash Connection");
    g_signal_connect(btn_test_connection, "clicked", G_CALLBACK(on_btn_test_connection), NULL);
    gtk_container_add(GTK_CONTAINER(window), btn_test_connection);

    // add debug button
    GtkWidget *btn_debug = gtk_button_new_with_label("Debug Button");
    g_signal_connect(btn_debug, "clicked", G_CALLBACK(on_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(window), btn_debug);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
