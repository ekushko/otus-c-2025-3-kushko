// /drivers/usb/core/          - USB core (you use this)
// /drivers/hid/usbhid/        - USB HID layer (you use this)
// /drivers/input/             - Input subsystem (you use this)
// /drivers/input/mouse/       - WHERE YOU PUT YOUR DRIVER

// 99% of your driver uses existing code. You only write:
// Device identification (vendor/product ID)
// Special button handling
// Maybe custom HID report parsing if your button is weird



static struct usb_device_id mouse_table[] = {  // Your mouse driver code
    { USB_DEVICE(0x1234, 0x5678) },            // Your vendor/product ID
    { }
};

static int mouse_probe(struct usb_interface *intf, 
                      const struct usb_device_id *id)
{
    
    if (has_special_button) {  // Handle your special button here
        
        input_set_capability(input_dev, EV_KEY, BTN_SPECIAL);  // Register additional input capability
    }
    
    return hid_generic_probe(intf, id);  // Use existing HID parsing for standard buttons
}

static void mouse_disconnect(struct usb_interface *intf)
{
    
    hid_generic_disconnect(intf);  // Cleanup your special button
}

static struct usb_driver mouse_driver = {
    .name = "my_special_mouse",
    .probe = mouse_probe,
    .disconnect = mouse_disconnect,
    .id_table = mouse_table,
};
