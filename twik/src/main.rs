extern crate libusb;

use std::io::{self, Write};
use std::time::Duration;

fn find_device<'a>(context: &'a libusb::Context) -> libusb::Result<libusb::Device<'a>> {
    for device in try!(context.devices()).iter() {
        let device_desc = try!(device.device_descriptor());
        println!("{:?}", device_desc);

        if device_desc.vendor_id() == 0x1209 && device_desc.product_id() == 0x8b00 {
            return Ok(device);
        }
    }
    Err(libusb::Error::NotFound)
}

fn find_interface_descriptor<'a>(config: &'a libusb::ConfigDescriptor) -> libusb::Result<libusb::InterfaceDescriptor<'a>> {
    for interface in config.interfaces() {
        for descriptor in interface.descriptors() {
            if descriptor.class_code() == 0xFF && descriptor.sub_class_code() == 0x00 {
                return Ok(descriptor);
            }
        }
    }
    Err(libusb::Error::NotFound)
}

fn main() {
    let context = std::boxed::Box::new(libusb::Context::new().unwrap());
    let device = find_device(&context).expect("could not find libtransistor device");
    let config = device.active_config_descriptor().unwrap();
    let interface_descriptor = find_interface_descriptor(&config).expect("could not find libtransistor interface");

    let mut endp_iter = interface_descriptor.endpoint_descriptors();
    let endp_data_out = endp_iter.next().unwrap();
    let endp_data_in = endp_iter.next().unwrap();
    assert!(endp_iter.next().is_none());


    assert_eq!(endp_data_out.direction(), libusb::Direction::Out);
    assert_eq!(endp_data_in.direction(), libusb::Direction::In);
    assert_eq!(endp_data_out.transfer_type(), libusb::TransferType::Bulk);
    assert_eq!(endp_data_in.transfer_type(), libusb::TransferType::Bulk);
    
    println!("found libtransistor endpoints!");
    
    let mut handle = device.open().unwrap();
    if handle.kernel_driver_active(interface_descriptor.interface_number()).unwrap() {
        println!("detaching kernel driver for libtransistor bridge");
        handle.detach_kernel_driver(interface_descriptor.interface_number()).unwrap();
    }
    handle.claim_interface(interface_descriptor.interface_number()).unwrap();
    println!("claimed libtransistor interface");

    loop {
        let mut data = [0; 0x4000];
        let siz = handle.read_bulk(endp_data_in.address(), &mut data[..], Duration::new(u64::max_value() / 1000, 0)).unwrap();
        io::stdout().write(&data[..siz]).unwrap();
    }
}
