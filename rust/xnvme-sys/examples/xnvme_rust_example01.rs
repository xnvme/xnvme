use std::process::ExitCode;

fn main() -> ExitCode {
    println!(
        "v{}.{}.{}",
        unsafe { xnvme_sys::xnvme_ver_major() },
        unsafe { xnvme_sys::xnvme_ver_minor() },
        unsafe { xnvme_sys::xnvme_ver_patch() },
    );

    ExitCode::from(0)
}
