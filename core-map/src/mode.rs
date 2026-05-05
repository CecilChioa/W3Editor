use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum EditorMode {
    Terrain,
    Doodad,
    Destructable,
    Region,
}

impl EditorMode {
    pub fn label(self) -> &'static str {
        match self {
            EditorMode::Terrain => "Terrain",
            EditorMode::Doodad => "Doodad",
            EditorMode::Destructable => "Destructable",
            EditorMode::Region => "Region",
        }
    }
}
