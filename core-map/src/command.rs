use crate::mode::EditorMode;
use serde::{Deserialize, Serialize};
use std::collections::VecDeque;

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct Vec3f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum EditorCommand {
    SetMode(EditorMode),
    PlaceDoodad {
        object_id: String,
        position: Vec3f,
        yaw: f32,
    },
    MoveSelection {
        delta: Vec3f,
    },
    DeleteSelection,
}

#[derive(Debug, Default)]
pub struct CommandBus {
    queue: VecDeque<EditorCommand>,
}

impl CommandBus {
    pub fn push(&mut self, command: EditorCommand) {
        self.queue.push_back(command);
    }

    pub fn pop(&mut self) -> Option<EditorCommand> {
        self.queue.pop_front()
    }

    pub fn len(&self) -> usize {
        self.queue.len()
    }

    pub fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }
}
