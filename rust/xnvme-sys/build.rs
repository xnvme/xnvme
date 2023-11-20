use std::error::Error;
use std::io;
use std::io::ErrorKind;
use std::path;

fn main() -> Result<(), Box<dyn Error>> {
    // Emit link options for linking system dependencies
    let deps = system_deps::Config::new().probe()?;
    let mut header_path = path::PathBuf::new();

    // Find library header
    for include_path in deps.all_include_paths().iter() {
        header_path.clear();
        header_path.clone_from(include_path);
        header_path.push("libxnvme.h");

        if header_path.exists() {
            break;
        }
    }

    if !header_path.exists() {
        return Err(Box::new(io::Error::new(
            ErrorKind::Other,
            "Cannot find header_file('libxnvme.h'); is xNVMe installed?".to_string(),
        )));
    }

    // Generate include arguments for clang
    let flags: Vec<String> = deps
        .all_include_paths()
        .iter()
        .map(|x| format!("-I{}", x.to_str().unwrap()))
        .collect();

    // Generate bindings for xnvme
    let bindings = bindgen::Builder::default()
        .header(header_path.to_str().map(|s| s.to_string()).unwrap())
        .allowlist_function("xnvme_.*")
        .allowlist_type("xnvme_.*")
        .clang_args(flags)
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Failed to generate bindings");

    // Write bindings to file
    let out_path = std::path::PathBuf::from(std::env::var("OUT_DIR")?);

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Failed to write bindings to file");

    Ok(())
}
