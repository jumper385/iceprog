#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>

// include iceprog library
#include "iceprog_fn.h"
#include "mpsse.h"

static bool mpsse_initialized = false;
static char *selected_file_path = NULL;
static GtkWidget *lbl_file_path = NULL;
static GtkWidget *progress_bar = NULL;

static void update_progress(double fraction, const char *text) {
    if (progress_bar) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), fraction);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), text);
        // Process pending GUI events to update the display
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }
}

static void cleanup_mpsse() {
    if (mpsse_initialized) {
        printf("Cleaning up MPSSE interface...\n");
        mpsse_close();
        mpsse_initialized = false;
    }
    if (selected_file_path) {
        g_free(selected_file_path);
        selected_file_path = NULL;
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

void on_btn_select_file(GtkButton *button, gpointer user_data) {
    printf("Selecting bitstream file...\n");
    // updates file path variable for flashing when pressed
    // Open file chooser dialog
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Bitstream File",
                                                    GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    // Show the dialog and wait for a response
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        printf("Selected file: %s\n", filename);
        
        // Update the file path variable for flashing
        if (selected_file_path) {
            g_free(selected_file_path);
        }
        selected_file_path = g_strdup(filename);
        
        // Update the label to show the selected file
        if (lbl_file_path) {
            gtk_label_set_text(GTK_LABEL(lbl_file_path), filename);
        }
        
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void on_btn_flash_chip(GtkButton *button, gpointer user_data) {
    printf("Flashing the chip...\n");
    
    // Check if a file is selected
    if (!selected_file_path) {
        printf("Error: No bitstream file selected!\n");
        update_progress(0.0, "Error: No file selected");
        return;
    }
    
    update_progress(0.0, "Opening file...");
    
    // Open the selected file
    FILE *f = fopen(selected_file_path, "rb");
    if (f == NULL) {
        printf("Error: Cannot open file '%s' for reading\n", selected_file_path);
        update_progress(0.0, "Error: Cannot open file");
        return;
    }
    
    // Get file size
    fseek(f, 0L, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    if (file_size <= 0) {
        printf("Error: Invalid file size\n");
        update_progress(0.0, "Error: Invalid file size");
        fclose(f);
        return;
    }
    
    printf("File size: %ld bytes\n", file_size);
    update_progress(0.05, "File loaded successfully");
    
    // Initialize MPSSE if not already done
    if (!mpsse_initialized) {
        update_progress(0.1, "Initializing MPSSE interface...");
        printf("Initializing MPSSE interface...\n");
        mpsse_init(0, NULL, false);
        mpsse_initialized = true;
        flash_release_reset();
        usleep(100000);
    }
    
    // Reset and prepare flash
    update_progress(0.15, "Preparing flash...");
    printf("Preparing flash...\n");
    flash_chip_deselect();
    usleep(250000);
    flash_reset();
    flash_power_up();
    flash_read_id();
    
    // Erase flash (using 64kB sectors)
    update_progress(0.2, "Erasing flash...");
    printf("Erasing flash...\n");
    int erase_block_size = 64; // 64kB sectors
    int block_size = erase_block_size << 10; // Convert to bytes
    int block_mask = block_size - 1;
    int begin_addr = 0 & ~block_mask;
    int end_addr = (file_size + block_mask) & ~block_mask;
    
    int total_erase_blocks = (end_addr - begin_addr) / block_size;
    int current_erase_block = 0;
    
    for (int addr = begin_addr; addr < end_addr; addr += block_size) {
        double erase_progress = 0.2 + (0.3 * current_erase_block / total_erase_blocks);
        char erase_text[100];
        snprintf(erase_text, sizeof(erase_text), "Erasing sector %d/%d", 
                current_erase_block + 1, total_erase_blocks);
        update_progress(erase_progress, erase_text);
        
        printf("Erasing sector at 0x%06X\n", addr);
        flash_write_enable();
        flash_64kB_sector_erase(addr);
        flash_wait();
        current_erase_block++;
    }
    
    // Program flash
    update_progress(0.5, "Programming flash...");
    printf("Programming flash...\n");
    for (int rc, addr = 0; true; addr += rc) {
        uint8_t buffer[256];
        int page_size = 256 - addr % 256;
        rc = fread(buffer, 1, page_size, f);
        if (rc <= 0)
            break;
            
        double prog_progress = 0.5 + (0.3 * addr / file_size);
        char prog_text[100];
        snprintf(prog_text, sizeof(prog_text), "Programming: %ld%% (0x%06X)", 
                100 * addr / file_size, addr);
        update_progress(prog_progress, prog_text);
        
        flash_write_enable();
        flash_prog(addr, buffer, rc);
        flash_wait();
    }
    
    // Verify programming
    update_progress(0.8, "Verifying flash...");
    printf("Verifying flash...\n");
    fseek(f, 0, SEEK_SET);
    bool verify_ok = true;
    for (int addr = 0; true; addr += 256) {
        uint8_t buffer_flash[256], buffer_file[256];
        int rc = fread(buffer_file, 1, 256, f);
        if (rc <= 0)
            break;
            
        double verify_progress = 0.8 + (0.15 * addr / file_size);
        char verify_text[100];
        snprintf(verify_text, sizeof(verify_text), "Verifying: %ld%% (0x%06X)", 
                100 * addr / file_size, addr);
        update_progress(verify_progress, verify_text);
        
        flash_read(addr, buffer_flash, rc);
        if (memcmp(buffer_file, buffer_flash, rc)) {
            printf("\nVerification failed at address 0x%06X!\n", addr);
            update_progress(0.95, "Verification failed!");
            verify_ok = false;
            break;
        }
    }
    
    // Power down flash
    update_progress(0.95, "Finalizing...");
    flash_power_down();
    flash_release_reset();
    usleep(250000);
    
    fclose(f);
    
    if (verify_ok) {
        printf("\nVERIFY OK\n");
        update_progress(1.0, "Flash completed successfully!");
        printf("Flash operation completed.\n");
    } else {
        update_progress(0.0, "Flash failed - verification error");
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IceProg GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create a vertical box to hold multiple widgets
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Add a button to test SPI flash connection
    GtkWidget *btn_test_connection = gtk_button_new_with_label("Test Probe Connection");
    g_signal_connect(btn_test_connection, "clicked", G_CALLBACK(on_btn_test_connection), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), btn_test_connection, TRUE, TRUE, 0);
    // Add bitstream file selection. file path selection dialog
    GtkWidget *btn_select_file = gtk_button_new_with_label("Select Bitstream File");
    g_signal_connect(btn_select_file, "clicked", G_CALLBACK(on_btn_select_file), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), btn_select_file, TRUE, TRUE, 0);

    // Selected file path display if present
    lbl_file_path = gtk_label_new("No file selected");
    gtk_box_pack_start(GTK_BOX(vbox), lbl_file_path, TRUE, TRUE, 0);

    // Progress bar for flashing operations
    progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Ready");
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0);

    // Flash the Chip
    GtkWidget *btn_flash_chip = gtk_button_new_with_label("Flash Chip");
    g_signal_connect(btn_flash_chip, "clicked", G_CALLBACK(on_btn_flash_chip), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), btn_flash_chip, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
