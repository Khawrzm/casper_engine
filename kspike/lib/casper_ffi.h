/*
 * casper_ffi.h - FFI Header for Casper Engine
 */
#ifndef CASPER_FFI_H
#define CASPER_FFI_H

#ifdef _WIN32
    #ifdef BUILDING_CASPER_DLL
        #define CASPER_API __declspec(dllexport)
    #else
        #define CASPER_API __declspec(dllimport)
    #endif
#else
    #define CASPER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

CASPER_API int casper_init(const char* config_json);
CASPER_API int casper_judge_evaluate(const char* req_json, char* out_buf, int out_len);
CASPER_API void casper_shutdown();

#ifdef __cplusplus
}
#endif

#endif // CASPER_FFI_H
