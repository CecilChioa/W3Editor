pub mod command;
pub mod doo;
pub mod mode;
pub mod mpq;
pub mod property;
pub mod w3e;
pub mod w3i;

use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MapBasicInfo {
    pub name: String,
    pub author: String,
    pub width: u32,
    pub height: u32,
    pub doodad_count: u32,
}

pub fn open_map_basic_info(path: &str) -> Result<MapBasicInfo, String> {
    if path.trim().is_empty() {
        return Err("map path is empty".to_string());
    }
    let p = Path::new(path);
    if !p.exists() {
        return Err(format!("map path not found: {}", p.display()));
    }

    if p.is_dir() {
        open_from_dir(p)
    } else {
        open_from_archive(path)
    }
}

fn open_from_dir(map_dir: &Path) -> Result<MapBasicInfo, String> {
    let w3e_data = fs::read(map_dir.join("war3map.w3e")).map_err(|e| format!("read w3e failed: {e}"))?;
    let doo_data = fs::read(map_dir.join("war3map.doo")).map_err(|e| format!("read doo failed: {e}"))?;
    let w3i_data = fs::read(map_dir.join("war3map.w3i")).ok();

    build_basic_info(
        map_dir.file_name().and_then(|x| x.to_str()).unwrap_or("Unknown Map"),
        &w3e_data,
        &doo_data,
        w3i_data.as_deref(),
    )
}

fn open_from_archive(path: &str) -> Result<MapBasicInfo, String> {
    let archive = mpq::MapArchive::open(path)?;
    let w3e_data = archive
        .get_file_ci("war3map.w3e")
        .ok_or_else(|| "war3map.w3e not found in map archive".to_string())?;
    let doo_data = archive
        .get_file_ci("war3map.doo")
        .ok_or_else(|| "war3map.doo not found in map archive".to_string())?;
    let w3i_data = archive.get_file_ci("war3map.w3i");

    let map_name = Path::new(path)
        .file_name()
        .and_then(|x| x.to_str())
        .unwrap_or("Unknown Map");

    build_basic_info(map_name, w3e_data, doo_data, w3i_data)
}

fn build_basic_info(
    fallback_name: &str,
    w3e_data: &[u8],
    doo_data: &[u8],
    w3i_data: Option<&[u8]>,
) -> Result<MapBasicInfo, String> {
    let w3e = w3e::W3eFile::parse(w3e_data).map_err(|e| format!("parse w3e failed: {e}"))?;
    let doo = doo::DooFile::parse(doo_data).map_err(|e| format!("parse doo failed: {e}"))?;

    let (name, author) = if let Some(data) = w3i_data {
        match w3i::W3iFile::parse(data) {
            Ok(v) => {
                let n = if v.map_name.is_empty() { fallback_name.to_string() } else { v.map_name };
                let a = if v.author.is_empty() { "Unknown Author".to_string() } else { v.author };
                (n, a)
            }
            Err(_) => (fallback_name.to_string(), "Unknown Author".to_string()),
        }
    } else {
        (fallback_name.to_string(), "Unknown Author".to_string())
    };

    Ok(MapBasicInfo {
        name,
        author,
        width: w3e.width,
        height: w3e.height,
        doodad_count: doo.doodad_count,
    })
}
