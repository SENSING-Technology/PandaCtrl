#ifndef VIDEO_DEVICE_INTERFACE_H
#define VIDEO_DEVICE_INTERFACE_H

#include <libusb-1.0/libusb.h>
#include <stdint.h>

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USBControlRequest;

typedef struct {
    libusb_device_handle *dev_handle;
    uint8_t interface_number;
} VideoDevice;

VideoDevice* video_device_open(uint16_t vendor_id, uint16_t product_id);
void video_device_close(VideoDevice *device);

int video_device_set_get_extension_unit(
    VideoDevice *device,
    uint8_t extension_node,
    uint16_t property_id,
    uint16_t flags,
    void *data,
    uint16_t len,
    uint16_t *read_count);

#endif // VIDEO_DEVICE_INTERFACE_H