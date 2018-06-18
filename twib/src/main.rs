extern crate libusb;
extern crate getopts;
extern crate byteorder;
extern crate rand;
extern crate prettytable;
extern crate rmpv;
extern crate mio;
extern crate tokio;

mod usb;
mod error;

use byteorder::{ReadBytesExt, WriteBytesExt, LittleEndian};
use std::io::Read;
use std::io::Write;
use std::sync::Arc;
use libusb::io::unix_async::*;
use prettytable::*;

fn mio_thread(context: Arc<Context>) {
    const USB: mio::Token = mio::Token(0);
    let poll = mio::Poll::new().expect("Create poll");
    poll.register(context.as_ref(), USB, mio::Ready::readable(), mio::PollOpt::level()).expect("Register USB");
    let mut events = mio::Events::with_capacity(1024);
    let mut v = Vec::new();
    loop {
        poll.poll(&mut events, None).unwrap();
        for event in events.iter() {
            match event.token() {
                USB => {
                    let _res = context.handle(&poll, &mut v);
                    v.clear();
                }
                _ => unreachable!(),
            }
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
    
    let twili_usb = usb::TwiliUSB::find(Arc::new(libusb::Context::new().unwrap()));
    
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
            let r = twili_usb.transact(usb::CommandId::RUN, content_buf.as_slice(), std::time::Duration::new(20, 0)).unwrap();
            let mut cursor:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(r);
            println!("PID: {:?}", cursor.read_u64::<LittleEndian>().unwrap());
        },
        "reboot" => {
            if matches.free.len() != 1 {
                panic!("usage: twib reboot");
            }
            twili_usb.transact(usb::CommandId::REBOOT, &[0], std::time::Duration::new(20, 0)).unwrap();
        },
        "coredump" => {
            if matches.free.len() != 2 {
                panic!("usage: twib coredump <file>");
            }
            let mut f = std::fs::File::create(&matches.free[1]).expect("could not create file");
            f.write_all(
                twili_usb.transact(usb::CommandId::COREDUMP, &[], std::time::Duration::new(20, 0))
                    .unwrap().as_slice()).unwrap();
        },
        "terminate" => {
            if matches.free.len() != 2 {
                panic!("usage: twib terminate <pid>");
            }
            let mut curs:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(Vec::new());
            curs.write_u64::<LittleEndian>(matches.free[1].parse::<u64>().unwrap()).unwrap();
            twili_usb.transact(usb::CommandId::TERMINATE, curs.get_ref(), std::time::Duration::new(20, 0)).unwrap();
        },
        "ps" => {
            if matches.free.len() != 1 {
                panic!("usage: twib ps");
            }
            let response = twili_usb.transact(usb::CommandId::LIST_PROCESSES, &Vec::new(), std::time::Duration::new(20, 0)).unwrap();
            let num_reports = response.len() / 40;
            let mut rcurs:std::io::Cursor<Vec<u8>> = std::io::Cursor::new(response);

            let mut table = prettytable::Table::new();

            table.add_row(row!["PID", "Debug Result", "TID", "Name", "MMU Flags"]);
            
            for _ in 0..num_reports {
                let process_id = rcurs.read_u64::<LittleEndian>().unwrap();
                let result = rcurs.read_u32::<LittleEndian>().unwrap();
                rcurs.read_u32::<LittleEndian>().unwrap(); // alignment
                let title_id = rcurs.read_u64::<LittleEndian>().unwrap();
                let mut process_name:[u8; 12] = [0; 12];
                rcurs.read_exact(&mut process_name).unwrap();
                let mmu_flags = rcurs.read_u32::<LittleEndian>().unwrap();

                table.add_row(row![format!("{}", process_id),
                                   format!("0x{:x}", result),
                                   format!("0x{:x}", title_id),
                                   String::from_utf8_lossy(&process_name),
                                   format!("0x{:x}", mmu_flags)]);
            }

            table.set_format(*format::consts::FORMAT_NO_BORDER_LINE_SEPARATOR);
            table.printstd();
        },
        "upgrade-twili" => {
            if matches.free.len() != 2 {
                panic!("usage: twib upgrade-twili <twili_launcher.nsp>");
            }
            let mut content_buf:Vec<u8> = Vec::new();
            {
                let mut f = std::fs::File::open(&matches.free[1]).expect("file not found");
                f.read_to_end(&mut content_buf).expect("failed to read from file");
            }
            twili_usb.transact(usb::CommandId::UPGRADE_TWILI, content_buf.as_slice(), std::time::Duration::new(20, 0)).unwrap();
        },
        "identify" => {
            if matches.free.len() != 1 {
                panic!("usage: twib identify");
            }
            let response = twili_usb.transact(usb::CommandId::IDENTIFY, &Vec::new(), std::time::Duration::new(20, 0)).unwrap();
            let v = rmpv::decode::value::read_value(&mut std::io::Cursor::new(response)).unwrap();
            for &(ref k, ref v) in v.as_map().unwrap() {
                println!("{} -> {:?}", k.as_str().unwrap(), v);
            }
        },
        _ => panic!("unknown operation: {:?}", matches.free[0])
    }

}
