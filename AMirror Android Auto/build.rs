fn main() {
	#[cfg(target_arch = "aarch64")] {
		let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
		println!("cargo:rustc-link-search=native={}/lib", dir);
		println!("cargo:rustc-link-lib=static=asound");
    }
}
