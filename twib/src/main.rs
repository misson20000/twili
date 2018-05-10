extern crate libusb;
extern crate getopts;
extern crate byteorder;
extern crate rand;

use byteorder::{ReadBytesExt, WriteBytesExt, LittleEndian};
use std::io::Read;
use std::io::Write;

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
    endpoint_meta_out: libusb::EndpointDescriptor<'a>,
    endpoint_data_out: libusb::EndpointDescriptor<'a>,
    endpoint_meta_in: libusb::EndpointDescriptor<'a>,
    endpoint_data_in: libusb::EndpointDescriptor<'a>,
}

enum TwiliUSBCommandId {
    RUN = 10,
    REBOOT = 11,
    COREDUMP = 12,
}

impl<'a> TwiliUSB<'a> {
    fn transact(&mut self, command_id: TwiliUSBCommandId, content_buf: &[u8], timeout: std::time::Duration) -> libusb::Result<(u32, std::vec::Vec<u8>)> {
        let tag:u32 = rand::random();
        { // request
            let mut header:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(Vec::new());
            header.write_u32::<LittleEndian>(command_id as u32).unwrap();
            header.write_u32::<LittleEndian>(tag).unwrap();
            header.write_u64::<LittleEndian>(content_buf.len() as u64).unwrap();
            try!(self.handle.write_bulk(self.endpoint_meta_out.address(), header.get_ref(), timeout));
            let mut bytes_written:usize = 0;
            while bytes_written < content_buf.len() {
                bytes_written+= try!(self.handle.write_bulk(self.endpoint_data_out.address(), &content_buf[bytes_written..], timeout));
                println!("bytes_written: {:?}", bytes_written);
            }
        }
        { // response
            let mut header_vec:Vec<u8> = Vec::new();
            header_vec.resize(16, 0);
            println!("awaiting response header");
            try!(self.handle.read_bulk(self.endpoint_meta_in.address(), header_vec.as_mut_slice(), timeout));
            let mut header_cursor:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(header_vec);
            let result_code = header_cursor.read_u32::<LittleEndian>().unwrap();
            let in_tag = header_cursor.read_u32::<LittleEndian>().unwrap();
            let response_size = header_cursor.read_u64::<LittleEndian>().unwrap() as usize;
            assert_eq!(in_tag, tag);
            println!("response size: {:?}", response_size);
            let mut response:Vec<u8> = Vec::new();
            if response_size > 0 {
                response.resize(response_size, 0);
                let mut bytes_read:usize = 0;
                while bytes_read < response_size {
                    let max_transfer_size = 0x80000;
                    let region:&mut [u8] = if response_size - bytes_read > max_transfer_size {
                        &mut response[bytes_read..(bytes_read + max_transfer_size)]
                    } else {
                        &mut response[bytes_read..]
                    };
                    bytes_read+= try!(self.handle.read_bulk(self.endpoint_data_in.address(), region, timeout));
                    println!("have {:?}/{:?} bytes", bytes_read, response_size);
                }
            }
            Ok((result_code, response))
        }
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    let opts = getopts::Options::new();
    let matches = opts.parse(&args[1..]).unwrap();
    if matches.free.len() < 1 {
        panic!("usage: twib <operation> args...");
    }
    
    let context = std::boxed::Box::new(libusb::Context::new().unwrap());
    let twili_device = find_twili_device(&context).expect("could not find Twili device");
    let twili_config = twili_device.active_config_descriptor().unwrap();
    let twili_interface_descriptor = find_twili_interface_descriptor(&twili_config).expect("could not find Twili interface");
    
    let mut endp_iter = twili_interface_descriptor.endpoint_descriptors();
    let endp_meta_out = endp_iter.next().unwrap();
    let endp_data_out = endp_iter.next().unwrap();
    let endp_meta_in = endp_iter.next().unwrap();
    let endp_data_in = endp_iter.next().unwrap();
    assert!(endp_iter.next().is_none());
    

    assert_eq!(endp_meta_out.direction(), libusb::Direction::Out);
    assert_eq!(endp_data_out.direction(), libusb::Direction::Out);
    assert_eq!(endp_meta_in.direction(), libusb::Direction::In);
    assert_eq!(endp_data_in.direction(), libusb::Direction::In);
    assert_eq!(endp_meta_out.transfer_type(), libusb::TransferType::Bulk);
    assert_eq!(endp_meta_out.transfer_type(), libusb::TransferType::Bulk);
    assert_eq!(endp_meta_in.transfer_type(), libusb::TransferType::Bulk);
    assert_eq!(endp_meta_in.transfer_type(), libusb::TransferType::Bulk);
    
    println!("found Twili endpoints!");
    
    let mut twili_handle = twili_device.open().unwrap();
    if twili_handle.kernel_driver_active(twili_interface_descriptor.interface_number()).unwrap() {
        println!("detaching kernel driver for Twili bridge");
        twili_handle.detach_kernel_driver(twili_interface_descriptor.interface_number()).unwrap();
    }
    twili_handle.claim_interface(twili_interface_descriptor.interface_number()).unwrap();
    println!("claimed Twili interface");
    
    let mut twili_usb = TwiliUSB { handle: twili_handle,
                                   endpoint_meta_out: endp_meta_out,
                                   endpoint_data_out: endp_data_out,
                                   endpoint_meta_in: endp_meta_in,
                                   endpoint_data_in: endp_data_in };
    
    match matches.free[0].as_ref() {
        "run" => {
            if matches.free.len() != 2 {
                panic!("usage: twib run <file>");
            }
            let mut content_buf:Vec<u8> = Vec::new();
            {
                let mut f = std::fs::File::open(&matches.free[1]).expect("file not found");
                f.read_to_end(&mut content_buf).expect("failed to read from file");
            }
            println!("returned {:?}", twili_usb.transact(TwiliUSBCommandId::RUN, content_buf.as_slice(), std::time::Duration::new(20, 0)).unwrap().0);
        },
        "reboot" => {
            if matches.free.len() != 1 {
                panic!("usage: twib reboot");
            }
            println!("returned {:?}", twili_usb.transact(TwiliUSBCommandId::REBOOT, &[0], std::time::Duration::new(20, 0)).unwrap().0);
        },
        "coredump" => {
            if matches.free.len() != 2 {
                panic!("usage: twib coredump <file>");
            }
            let mut f = std::fs::File::create(&matches.free[1]).expect("could not create file");
            let r = twili_usb.transact(TwiliUSBCommandId::COREDUMP, &[], std::time::Duration::new(20, 0)).unwrap();
            if r.0 != 0 {
                panic!("  got back code {:?}", r.0);
            }
            f.write_all(r.1.as_slice()).unwrap();
        },
        _ => panic!("unknown operation: {:?}", matches.free[0])
    }

}
