use byteorder::{LittleEndian, ReadBytesExt};
use serde::{Deserialize, Serialize};
use std::io::{Cursor, Read};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct W3iFile {
    pub map_name: String,
    pub author: String,
}

impl W3iFile {
    pub fn parse(data: &[u8]) -> Result<Self, String> {
        let mut cur = Cursor::new(data);
        let _version = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let _saves = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;
        let _editor = cur.read_u32::<LittleEndian>().map_err(|e| e.to_string())?;

        let map_name = read_cstring(&mut cur)?;
        let author = read_cstring(&mut cur)?;
        Ok(Self { map_name, author })
    }
}

fn read_cstring(cur: &mut Cursor<&[u8]>) -> Result<String, String> {
    let mut out = Vec::new();
    loop {
        let mut b = [0u8; 1];
        cur.read_exact(&mut b).map_err(|e| e.to_string())?;
        if b[0] == 0 {
            break;
        }
        out.push(b[0]);
    }
    Ok(String::from_utf8_lossy(&out).to_string())
}
