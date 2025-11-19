#include "pccore.h" // For IMAGE, PCCORE, VIDEOMODE, and render() prototype
#include "cga.h"    // For render320x200x2 and render640x200x1 prototypes

#include <stdio.h> // For placeholder debug messages

/**
 * @brief Renders the PC core's memory into an image buffer.
 *
 * This function reads the state of the PC (especially its video memory,
 * based on the current 'mode') and renders the corresponding
 * graphical output into the provided IMAGE structure.
 *
 * This implementation acts as a dispatcher, calling the
 * appropriate rendering function based on the current video mode.
 *
 * @param image  A pointer to the IMAGE structure to be filled with
 * pixel data.
 * @param pccore The current state of the PC core to render from.
 * (Passed by value as per pccore.h)
 */
void render(IMAGE* image, PCCORE pccore) {
    if (image == NULL) {
        return; // Safety check: do nothing if image is null
    }

    // Dispatch to the correct rendering function based on the mode.
    // We pass a pointer to pccore to the sub-functions
    // to avoid copying the large struct again.
    switch (pccore.mode) {
        case CGA320x200x2:
            // Call the specific function for 320x200x2 mode
            render320x200x2(image, pccore);
            break;

        case CGA640x200x1:
            // Call the specific function for 640x200x1 mode
            render640x200x1(image, pccore);
            break;

        default:
            // Handle unknown or unsupported mode
            // We can clear the image or just log an error.
            printf("Unknown video mode requested: %d\n", pccore.mode);
            image->width = 0;
            image->height = 0;
            break;
    }
}
