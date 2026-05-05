use ceres_mpq::Archive;
use std::collections::HashMap;
use std::io::Cursor;

pub struct MapArchive {
    files: HashMap<String, Vec<u8>>,
}

impl MapArchive {
    pub fn open(path: &str) -> Result<Self, String> {
        let raw = std::fs::read(path).map_err(|e| format!("read map failed: {e}"))?;
        let mut cursor = Cursor::new(raw);
        let mut archive = Archive::open(&mut cursor).map_err(|e| format!("open mpq failed: {e:?}"))?;

        let listfile_raw = archive
            .read_file("(listfile)")
            .map_err(|_| "cannot read (listfile), map may be protected".to_string())?;

        let listfile_str = String::from_utf8_lossy(&listfile_raw);
        let mut files = HashMap::new();
        for name in listfile_str.lines().map(|x| x.trim()).filter(|x| !x.is_empty()) {
            if let Ok(data) = archive.read_file(name) {
                files.insert(name.to_string(), data);
            }
        }
        if !files.contains_key("(listfile)") {
            files.insert("(listfile)".to_string(), listfile_raw);
        }

        Ok(Self { files })
    }

    pub fn get_file_ci(&self, name: &str) -> Option<&[u8]> {
        let needle = name.to_lowercase().replace('\\', "/");
        self.files
            .iter()
            .find(|(k, _)| k.to_lowercase().replace('\\', "/") == needle)
            .map(|(_, v)| v.as_slice())
    }
}
