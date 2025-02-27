set_project("SakuraTool")

if(has_config("build_usdtool")) then
    add_requires("vcpkg::usd", {configs={shared=true }})
end

target("SkrTool")
    set_kind("shared")
    add_files("core/src/**.cpp")
    add_deps("SkrRT")
    add_defines("TOOL_IMPL", {public = false})
    add_includedirs("core/include", {public = true})
    add_rules("c++.reflection", {
        files = {"core/**.h", "core/**.hpp"},
        rootdir = "core/",
        api = "TOOL"
    })

if(has_config("build_usdtool")) then
target("UsdTool")
    set_kind("shared")
    add_rules("c++.reflection", {
        files = {"usdtool/**.h", "usdtool/**.hpp"},
        rootdir = "usdtool/"
    })
    add_includedirs("usdtool/include", {public=true})
    add_packages("vcpkg::usd")
    add_defines("USDTOOL_SHARED", {public=true})
    add_defines("USDTOOL_IMPL")
    add_deps("SkrTool", "GameRT")
    add_files("usdtool/src/**.cpp")
end 

target("RenderTool")
    set_kind("shared")
    add_rules("c++.reflection", {
        files = {"rendertool/**.h", "rendertool/**.hpp"},
        rootdir = "rendertool/"
    })
    add_includedirs("rendertool/include", {public=true})
    add_packages("vcpkg::usd")
    add_defines("RENDERTOOL_SHARED", {public=true})
    add_defines("RENDERTOOL_IMPL")
    add_deps("SkrTool", "GameRT")
    add_files("rendertool/src/**.cpp")

target("SkrCompiler")
    -- add_rules("grpc.cpp")
    set_kind("binary")
    add_deps("GameTool", "SkrTool")
    -- add_files("compiler/**.proto")
    add_files("compiler/**.cpp")
    -- add_links("absl_bad_any_cast_impl","absl_bad_optional_access","absl_bad_variant_access","absl_base","absl_city","absl_civil_time","absl_cord","absl_cordz_functions","absl_cordz_handle","absl_cordz_info","absl_cordz_sample_token","absl_cord_internal","absl_debugging_internal","absl_demangle_internal","absl_examine_stack","absl_exponential_biased","absl_failure_signal_handler","absl_flags","absl_flags_commandlineflag","absl_flags_commandlineflag_internal","absl_flags_config","absl_flags_internal","absl_flags_marshalling","absl_flags_parse","absl_flags_private_handle_accessor","absl_flags_program_name","absl_flags_reflection","absl_flags_usage","absl_flags_usage_internal","absl_graphcycles_internal","absl_hash","absl_hashtablez_sampler","absl_int128","absl_leak_check","absl_leak_check_disable","absl_log_severity","absl_low_level_hash","absl_malloc_internal","absl_periodic_sampler","absl_random_distributions","absl_random_internal_distribution_test_util","absl_random_internal_platform","absl_random_internal_pool_urbg","absl_random_internal_randen","absl_random_internal_randen_hwaes","absl_random_internal_randen_hwaes_impl","absl_random_internal_randen_slow","absl_random_internal_seed_material","absl_random_seed_gen_exception","absl_random_seed_sequences","absl_raw_hash_set","absl_raw_logging_internal","absl_scoped_set_env","absl_spinlock_wait","absl_stacktrace","absl_status","absl_statusor","absl_strerror","absl_strings","absl_strings_internal","absl_str_format_internal","absl_symbolize","absl_synchronization","absl_throw_delegate","absl_time","absl_time_zone","address_sorting","cares","crypto","gpr","grpc++","grpc++_alts","grpc++_error_details","grpc++_reflection","grpc++_unsecure","grpc","grpcpp_channelz","grpc_plugin_support","grpc_unsecure","libprotobuf-lite","libprotobuf","libprotoc","re2","ssl","upb")
    
    if(has_config("build_usdtool")) then
        add_defines("WITH_USDTOOL", {public = false})
    end
    --if(is_os("windows")) then 
    --    add_links("zlibstatic")
    --end
    add_rules("c++.reflection", {
        files = {"compiler/**.h", "compiler/**.hpp"},
        rootdir = "compiler/"
    })