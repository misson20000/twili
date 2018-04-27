extern crate libusb;
extern crate getopts;
extern crate byteorder;

use byteorder::{ReadBytesExt, WriteBytesExt, BigEndian, LittleEndian};
use std::io::Read;

fn find_twili_device<'a>(context: &'a libusb::Context) -> libusb::Result<libusb::Device<'a>> {
    for mut device in try!(context.devices()).iter() {
        let device_desc = try!(device.device_descriptor());

        if device_desc.vendor_id() == 0x1209 && device_desc.product_id() == 0x8b00 {
            return Ok(device);
        }
    }
    Err(libusb::Error::NotFound)
}

fn find_twili_interface_descriptor<'a>(config: &'a libusb::ConfigDescriptor) -> libusb::Result<libusb::InterfaceDescriptor<'a>> {
    for interface in config.interfaces() {
        for descriptor in interface.descriptors() {
            if descriptor.class_code() == 0xFF && descriptor.sub_class_code() == 0x01 {
                return Ok(descriptor);
            }
        }
    }
    Err(libusb::Error::NotFound)
}

struct TwiliUSB<'a> {
    handle: libusb::DeviceHandle<'a>,
    endpoint_out: libusb::EndpointDescriptor<'a>,
    endpoint_in: libusb::EndpointDescriptor<'a>,
}

enum TwiliUSBCommandId {
    RUN = 10,
    REBOOT = 11,
}

impl<'a> TwiliUSB<'a> {
    fn write(&mut self, buf: &[u8], timeout: std::time::Duration) -> libusb::Result<usize> {
        println!("writing {:?} bytes", buf.len());
        self.handle.write_bulk(self.endpoint_out.address(), buf, timeout)
    }
    fn read(&mut self, buf: &mut [u8], timeout: std::time::Duration) -> libusb::Result<usize> {
        self.handle.read_bulk(self.endpoint_in.address(), buf, timeout)
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();

    let mut opts = getopts::Options::new();
    let matches = opts.parse(&args[1..]).unwrap();
    if matches.free.len() < 1 {
        panic!("usage: twib <operation> args...");
    }
    
    let context = std::boxed::Box::new(libusb::Context::new().unwrap());
    let twili_device = find_twili_device(&context).expect("could not find Twili device");
    let twili_config = twili_device.active_config_descriptor().unwrap();
    let twili_interface_descriptor = find_twili_interface_descriptor(&twili_config).expect("could not find Twili interface");
    
    let mut endp_iter = twili_interface_descriptor.endpoint_descriptors();
    let endp_out = endp_iter.next().unwrap();
    let endp_in = endp_iter.next().unwrap();
    assert!(endp_iter.next().is_none());
    
    assert_eq!(endp_in.direction(), libusb::Direction::In);
    assert_eq!(endp_out.direction(), libusb::Direction::Out);
    assert_eq!(endp_in.transfer_type(), libusb::TransferType::Bulk);
    assert_eq!(endp_out.transfer_type(), libusb::TransferType::Bulk);
    
    println!("found Twili endpoints!");
    println!("  in: {:?}", endp_in);
    println!("  out: {:?}", endp_out);
    
    let mut twili_handle = twili_device.open().unwrap();
    if twili_handle.kernel_driver_active(twili_interface_descriptor.interface_number()).unwrap() {
        println!("detaching kernel driver for Twili bridge");
        twili_handle.detach_kernel_driver(twili_interface_descriptor.interface_number()).unwrap();
    }
    twili_handle.claim_interface(twili_interface_descriptor.interface_number()).unwrap();
    println!("claimed Twili interface");
    
    let mut twili_usb = TwiliUSB { handle: twili_handle, endpoint_out: endp_out, endpoint_in: endp_in };
    match matches.free[0].as_ref() {
        "run" => {
            if matches.free.len() != 2 {
                panic!("usage: twib run <file>");
            }
            let mut content_buf:Vec<u8> = Vec::new();
            let size = {
                let mut f = std::fs::File::open(&matches.free[1]).expect("file not found");
                f.read_to_end(&mut content_buf).expect("failed to read from file")
            };
            let mut header:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(Vec::new());
            header.write_u64::<LittleEndian>(TwiliUSBCommandId::RUN as u64);
            header.write_u64::<LittleEndian>(size as u64);
            twili_usb.write(header.get_ref(), std::time::Duration::new(20, 0)).expect("failed to write transaction header");
            twili_usb.write(content_buf.as_slice(), std::time::Duration::new(20, 0)).expect("failed to write program");
        },
        "reboot" => {
            if matches.free.len() != 1 {
                panic!("usage: twib reboot");
            }
            let mut header:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(Vec::new());
            header.write_u64::<LittleEndian>(TwiliUSBCommandId::REBOOT as u64);
            header.write_u64::<LittleEndian>(0 as u64);
            twili_usb.write(header.get_ref(), std::time::Duration::new(20, 0)).expect("failed to write transaction header");            
        }
        _ => panic!("unknown operation: {:?}", matches.free[0])
    }

}
