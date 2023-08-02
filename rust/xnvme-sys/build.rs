fn main() {
    // Emit link options for linking system dependencies
    let deps = system_deps::Config::new().probe().unwrap();

    // Generate include arguments for clang
    let flags: Vec<String> = deps
        .all_include_paths()
        .iter()
        .map(|x| format!("-I{}", x.to_str().unwrap()))
        .collect();

    // Generate bindings for xnvme
    let bindings = bindgen::Builder::default()
        .header("../../include/libxnvme.h")
        .allowlist_function("xnvme_.*")
        .allowlist_type("xnvme_.*")
        .clang_args(flags)
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Failed to generate bindings");

    // Write bindings to file
    let out_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Failed to write bindings to file");
}
