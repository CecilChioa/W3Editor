use byteorder::{LittleEndian, ReadBytesExt};
use serde::{Deserialize, Serialize};
use std::io::{Cursor, Read};

const W3E_MAGIC: &[u8; 4] = b"W3E!";

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct W3eFile {
    pub width: u32,
    pub height: u32,
}

impl W3eFile {
    pub fn parse(data: &[u8]) -> Result<Self, String> {
        let mut cur = Cursor::new(data);
        let mut magic = [0u8; 4];
        cur.read_exact(&mut magic).map_err(|e| e.to_string())?;
        if &magic != W3E_MAGIC {
            return Err("invalid w3e header".to_string());
        }

        let _version = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let _main_tileset = cur.read_u8().map_err(|e| e.to_string())?;
        let _custom_tilesets = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;

        let ground_count = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        for _ in 0..ground_count {
            let mut id = [0u8; 4];
            cur.read_exact(&mut id).map_err(|e| e.to_string())?;
        }

        let cliff_count = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        for _ in 0..cliff_count {
            let mut id = [0u8; 4];
            cur.read_exact(&mut id).map_err(|e| e.to_string())?;
        }

        let width = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let height = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;

        Ok(Self { width, height })
    }
}
