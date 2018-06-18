use std;
use libusb;

use std::error::Error;

#[derive(Debug)]
pub enum TwiliError {
    InvalidNro,
    NoCrashedProcesses,
    ResponseSizeMismatch,
    FatalUsbTransfer,
    UsbTransfer,
    BadRequest,
    UnrecognizedPid,
    UnrecognizedHandlePlaceholder,
    InvalidSegment,
    IoError,

    Unknown(u32),
}

impl std::error::Error for TwiliError {
    fn description(&self) -> &str {
        match *self {
            TwiliError::InvalidNro => "invalid nro",
            TwiliError::NoCrashedProcesses => "no crashed processes",
            TwiliError::ResponseSizeMismatch => "response size mismatch",
            TwiliError::FatalUsbTransfer => "fatal USB transfer error",
            TwiliError::UsbTransfer => "USB transfer error",
            TwiliError::BadRequest => "bad request",
            TwiliError::UnrecognizedPid => "unrecognized pid",
            TwiliError::UnrecognizedHandlePlaceholder => "unrecognized handle placeholder",
            TwiliError::InvalidSegment => "invalid segment",
            TwiliError::IoError => "IO error",
            
            TwiliError::Unknown(rc) => "unknown",
        }
    }

    fn cause(&self) -> Option<&std::error::Error> {
        None
    }
}

impl std::fmt::Display for TwiliError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.write_str(self.description())
    }
}

impl TwiliError {
    pub fn from_code(code:u32) -> TwiliError {
        if (code & 0x1FF) != 0xEF {
            return TwiliError::Unknown(code);
        }
        match code >> 9 {
            1 => TwiliError::InvalidNro,
            2 => TwiliError::NoCrashedProcesses,
            3 => TwiliError::ResponseSizeMismatch,
            4 => TwiliError::FatalUsbTransfer,
            5 => TwiliError::UsbTransfer,
            6 => TwiliError::BadRequest,
            7 => TwiliError::UnrecognizedPid,
            8 => TwiliError::UnrecognizedHandlePlaceholder,
            9 => TwiliError::InvalidSegment,
            10 => TwiliError::IoError,
            _ => TwiliError::Unknown(code)
        }
    }

}

#[derive(Debug)]
pub enum BridgeError {
    Usb(libusb::Error),
    Remote(TwiliError),
    Io(std::io::Error),
}

impl std::error::Error for BridgeError {
    fn description(&self) -> &str {
        match *self {
            BridgeError::Usb(ref err) => err.description(),
            BridgeError::Remote(ref err) => err.description(),
            BridgeError::Io(ref err) => err.description(),
        }
    }

    fn cause(&self) -> Option<&std::error::Error> {
        match *self {
            BridgeError::Usb(ref err) => Some(err),
            BridgeError::Remote(ref err) => Some(err),
            BridgeError::Io(ref err) => Some(err),
        }
    }
}

impl std::fmt::Display for BridgeError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self.cause() {
            Some(ref err) => err.fmt(f),
            None => write!(f, "unknown bridge error")
        }
    }
}

impl std::convert::From<libusb::Error> for BridgeError {
    fn from(err: libusb::Error) -> BridgeError {
        BridgeError::Usb(err)
    }
}

impl std::convert::From<TwiliError> for BridgeError {
    fn from(err: TwiliError) -> BridgeError {
        BridgeError::Remote(err)
    }
}

impl std::convert::From<std::io::Error> for BridgeError {
    fn from(err: std::io::Error) -> BridgeError {
        BridgeError::Io(err)
    }
}

pub type BridgeResult<T> = Result<T, BridgeError>;
