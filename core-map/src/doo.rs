use byteorder::{LittleEndian, ReadBytesExt};
use serde::{Deserialize, Serialize};
use std::io::{Cursor, Read};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DooFile {
    pub doodad_count: u32,
}

impl DooFile {
    pub fn parse(data: &[u8]) -> Result<Self, String> {
        let mut cur = Cursor::new(data);
        let mut magic = [0u8; 4];
        cur.read_exact(&mut magic).map_err(|e| e.to_string())?;
        if &magic != b"W3do" {
            return Err("invalid doo header".to_string());
        }

        let _version = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let _subversion = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let count = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        Ok(Self { doodad_count: count })
    }
}
