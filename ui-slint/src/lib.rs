slint::include_modules!();

pub fn run_ui() -> Result<(), slint::PlatformError> {
    let ui = MainWindow::new()?;
    ui.run()
}
