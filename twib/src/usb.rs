use rand;
use std;
use libusb;
use error;

use byteorder::{ReadBytesExt, WriteBytesExt, LittleEndian};
use std::sync::Arc;
use libusb::io::unix_async::*;

fn find_twili_device<'a>(context: &'a Context) -> libusb::Result<Device<'a>> {
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

pub struct TwiliUSB<'a> {
    handle: DeviceHandle<'a>,
    endpoint_meta_out: libusb::EndpointDescriptor<'a>,
    endpoint_data_out: libusb::EndpointDescriptor<'a>,
    endpoint_meta_in: libusb::EndpointDescriptor<'a>,
    endpoint_data_in: libusb::EndpointDescriptor<'a>,
}

pub enum CommandId {
    RUN = 10,
    REBOOT = 11,
    COREDUMP = 12,
    TERMINATE = 13,
    LIST_PROCESSES = 14,
    UPGRADE_TWILI = 15,
    IDENTIFY = 16,
}

impl<'a> TwiliUSB<'a> {
    pub fn find(context:Arc<Context>) -> TwiliUSB<'a> {
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
        
        TwiliUSB { handle: twili_handle,
                   endpoint_meta_out: endp_meta_out,
                   endpoint_data_out: endp_data_out,
                   endpoint_meta_in: endp_meta_in,
                   endpoint_data_in: endp_data_in }
    }
    
    pub fn transact(&mut self, command_id: CommandId, content_buf: &[u8], timeout: std::time::Duration) -> error::BridgeResult<std::vec::Vec<u8>> {
        let max_transfer_size = 0x80000;
        let tag:u32 = rand::random();
        { // request
            let mut header:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(Vec::new());
            try!(header.write_u32::<LittleEndian>(command_id as u32));
            try!(header.write_u32::<LittleEndian>(tag));
            try!(header.write_u64::<LittleEndian>(content_buf.len() as u64));
            try!(self.handle.write_bulk(self.endpoint_meta_out.address(), header.get_ref(), timeout));
            let mut bytes_written:usize = 0;
            while bytes_written < content_buf.len() {
                let region:&[u8] = if content_buf.len() - bytes_written > max_transfer_size {
                    &content_buf[bytes_written..(bytes_written + max_transfer_size)]
                } else {
                    &content_buf[bytes_written..]
                };
                bytes_written+= try!(self.handle.write_bulk(self.endpoint_data_out.address(), region, timeout));
                println!("bytes_written: {:?}", bytes_written);
            }
        }
        { // response
            let mut header_vec:Vec<u8> = Vec::new();
            header_vec.resize(16, 0);
            println!("awaiting response header");
            try!(self.handle.read_bulk(self.endpoint_meta_in.address(), header_vec.as_mut_slice(), timeout));
            let mut header_cursor:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(header_vec);
            let result_code = try!(header_cursor.read_u32::<LittleEndian>());
            let in_tag = try!(header_cursor.read_u32::<LittleEndian>());
            let response_size = try!(header_cursor.read_u64::<LittleEndian>()) as usize;
            assert_eq!(in_tag, tag);
            println!("response size: {:?}", response_size);
            let mut response:Vec<u8> = Vec::new();
            if response_size > 0 {
                response.resize(response_size, 0);
                let mut bytes_read:usize = 0;
                while bytes_read < response_size {
                    let region:&mut [u8] = if response_size - bytes_read > max_transfer_size {
                        &mut response[bytes_read..(bytes_read + max_transfer_size)]
                    } else {
                        &mut response[bytes_read..]
                    };
                    bytes_read+= try!(self.handle.read_bulk(self.endpoint_data_in.address(), region, timeout));
                    println!("have {:?}/{:?} bytes", bytes_read, response_size);
                }
            }
            if result_code != 0 {
                Err(error::BridgeError::from(error::TwiliError::from_code(result_code)))
            } else {
                Ok(response)
            }
        }
    }
}
