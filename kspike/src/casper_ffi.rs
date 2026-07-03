use std::os::raw::{c_char, c_int};

// استدعاء الدوال من مكتبة Casper Engine C-ABI
#[link(name = "casper")]
extern "C" {
    pub fn casper_init(config_json: *const c_char) -> c_int;
    pub fn casper_judge_evaluate(req_json: *const c_char, out_buf: *mut c_char, out_len: c_int) -> c_int;
    pub fn casper_shutdown();
}
