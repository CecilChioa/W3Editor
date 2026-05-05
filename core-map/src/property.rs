use crate::mode::EditorMode;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum PropertyValueKind {
    Bool,
    Int,
    Float,
    Text,
    Enum(Vec<String>),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PropertyField {
    pub key: &'static str,
    pub label: &'static str,
    pub kind: PropertyValueKind,
}

pub fn schema_for_mode(mode: EditorMode) -> Vec<PropertyField> {
    match mode {
        EditorMode::Terrain => vec![
            PropertyField {
                key: "brush_size",
                label: "Brush Size",
                kind: PropertyValueKind::Float,
            },
            PropertyField {
                key: "brush_strength",
                label: "Brush Strength",
                kind: PropertyValueKind::Float,
            },
            PropertyField {
                key: "tile_id",
                label: "Tile ID",
                kind: PropertyValueKind::Text,
            },
        ],
        EditorMode::Doodad => vec![
            PropertyField {
                key: "object_id",
                label: "Object ID",
                kind: PropertyValueKind::Text,
            },
            PropertyField {
                key: "scale",
                label: "Scale",
                kind: PropertyValueKind::Float,
            },
            PropertyField {
                key: "yaw",
                label: "Yaw",
                kind: PropertyValueKind::Float,
            },
        ],
        EditorMode::Destructable => vec![
            PropertyField {
                key: "object_id",
                label: "Object ID",
                kind: PropertyValueKind::Text,
            },
            PropertyField {
                key: "life",
                label: "Life",
                kind: PropertyValueKind::Int,
            },
        ],
        EditorMode::Region => vec![
            PropertyField {
                key: "region_name",
                label: "Region Name",
                kind: PropertyValueKind::Text,
            },
            PropertyField {
                key: "weather",
                label: "Weather",
                kind: PropertyValueKind::Enum(vec![
                    "None".to_string(),
                    "Rain".to_string(),
                    "Snow".to_string(),
                    "Wind".to_string(),
                ]),
            },
            PropertyField {
                key: "ambient_sound",
                label: "Ambient Sound",
                kind: PropertyValueKind::Text,
            },
        ],
    }
}
