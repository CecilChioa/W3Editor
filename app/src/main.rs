use core_map::open_map_basic_info;
use rfd::FileDialog;
use slint::ComponentHandle;
use std::cell::RefCell;
use std::rc::Rc;
use ui_slint::MainWindow;

fn main() -> Result<(), String> {
    let ui = MainWindow::new().map_err(|e| format!("init ui failed: {e}"))?;

    let selected_path = Rc::new(RefCell::new(String::new()));

    {
        let ui_handle = ui.as_weak();
        let selected_path = Rc::clone(&selected_path);
        ui.on_choose_map(move || {
            if let Some(path) = FileDialog::new()
                .add_filter("War3 Map", &["w3x", "w3m"])
                .pick_file()
            {
                let path_str = path.to_string_lossy().to_string();
                *selected_path.borrow_mut() = path_str.clone();
                if let Some(ui) = ui_handle.upgrade() {
                    ui.set_selected_path(path_str.into());
                }
            }
        });
    }

    {
        let ui_handle = ui.as_weak();
        let selected_path = Rc::clone(&selected_path);
        ui.on_open_map(move || {
            let path = selected_path.borrow().clone();
            if path.is_empty() {
                if let Some(ui) = ui_handle.upgrade() {
                    ui.set_result_text("请先选择地图文件".into());
                }
                return;
            }

            let result_text = match open_map_basic_info(&path) {
                Ok(info) => format!(
                    "打开成功\n地图: {}\n作者: {}\n尺寸: {} x {}\n装饰物: {}",
                    info.name, info.author, info.width, info.height, info.doodad_count
                ),
                Err(err) => format!("打开失败\n路径: {}\n错误: {}", path, err),
            };

            if let Some(ui) = ui_handle.upgrade() {
                ui.set_result_text(result_text.into());
            }
        });
    }

    ui.run().map_err(|e| format!("run ui failed: {e}"))
}
