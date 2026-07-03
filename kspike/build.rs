fn main() {
    // توجيه المترجم لمجلد المكتبات
    let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    println!("cargo:rustc-link-search=native={}/lib", dir);
    
    // ربط المحرك (casper.lib)
    println!("cargo:rustc-link-lib=casper");
    
    // إعادة البناء فقط إذا تغيرت المكتبة
    println!("cargo:rerun-if-changed=lib/casper.lib");
}
