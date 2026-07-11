using UnrealBuildTool;
using System.IO;

public class CefProtobuf : ModuleRules
{
    public CefProtobuf(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );

        string thirdPartyRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "ThirdParty"));
        string protobufRoot = Path.Combine(thirdPartyRoot, "Protobuf");
        string protobufIncludePath = Path.Combine(protobufRoot, "include");
        string protobufCanonicalHeaderPath = Path.Combine(protobufIncludePath, "google", "protobuf", "runtime_version.h");

        if (!Directory.Exists(protobufRoot))
        {
            throw new BuildException($"CefProtobuf: protobuf root not found: {protobufRoot}");
        }

        if (!Directory.Exists(protobufIncludePath))
        {
            throw new BuildException($"CefProtobuf: protobuf include path not found: {protobufIncludePath}");
        }

        if (!File.Exists(protobufCanonicalHeaderPath))
        {
            throw new BuildException(
                "CefProtobuf: protobuf headers are not in the expected layout. " +
                $"Expected generated C++ includes like google/protobuf/... to resolve via: {protobufCanonicalHeaderPath}. " +
                $"Current vendor package appears flattened under: {protobufIncludePath}"
            );
        }

        PublicSystemIncludePaths.Add(protobufIncludePath);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string protobufLibDirectory = Path.Combine(protobufRoot, "lib", "Win64");
            string protobufLibPath = Path.Combine(protobufLibDirectory, "libprotobuf-lite.lib");

            if (!File.Exists(protobufLibPath))
            {
                throw new BuildException($"CefProtobuf: protobuf library not found: {protobufLibPath}");
            }

            string[] protobufDependencyLibraries =
            {
                "absl_base.lib",
                "absl_city.lib",
                "absl_civil_time.lib",
                "absl_cord.lib",
                "absl_cord_internal.lib",
                "absl_cordz_functions.lib",
                "absl_cordz_handle.lib",
                "absl_cordz_info.lib",
                "absl_cordz_sample_token.lib",
                "absl_crc_cord_state.lib",
                "absl_crc_cpu_detect.lib",
                "absl_crc_internal.lib",
                "absl_crc32c.lib",
                "absl_debugging_internal.lib",
                "absl_decode_rust_punycode.lib",
                "absl_demangle_internal.lib",
                "absl_demangle_rust.lib",
                "absl_die_if_null.lib",
                "absl_examine_stack.lib",
                "absl_exponential_biased.lib",
                "absl_failure_signal_handler.lib",
                "absl_flags_commandlineflag.lib",
                "absl_flags_commandlineflag_internal.lib",
                "absl_flags_config.lib",
                "absl_flags_internal.lib",
                "absl_flags_marshalling.lib",
                "absl_flags_parse.lib",
                "absl_flags_private_handle_accessor.lib",
                "absl_flags_program_name.lib",
                "absl_flags_reflection.lib",
                "absl_flags_usage.lib",
                "absl_flags_usage_internal.lib",
                "absl_graphcycles_internal.lib",
                "absl_hash.lib",
                "absl_hashtablez_sampler.lib",
                "absl_int128.lib",
                "absl_kernel_timeout_internal.lib",
                "absl_leak_check.lib",
                "absl_log_flags.lib",
                "absl_log_globals.lib",
                "absl_log_initialize.lib",
                "absl_log_internal_check_op.lib",
                "absl_log_internal_conditions.lib",
                "absl_log_internal_fnmatch.lib",
                "absl_log_internal_format.lib",
                "absl_log_internal_globals.lib",
                "absl_log_internal_log_sink_set.lib",
                "absl_log_internal_message.lib",
                "absl_log_internal_nullguard.lib",
                "absl_log_internal_proto.lib",
                "absl_log_internal_structured_proto.lib",
                "absl_log_severity.lib",
                "absl_log_sink.lib",
                "absl_low_level_hash.lib",
                "absl_malloc_internal.lib",
                "absl_periodic_sampler.lib",
                "absl_poison.lib",
                "absl_random_distributions.lib",
                "absl_random_internal_distribution_test_util.lib",
                "absl_random_internal_entropy_pool.lib",
                "absl_random_internal_platform.lib",
                "absl_random_internal_randen.lib",
                "absl_random_internal_randen_hwaes.lib",
                "absl_random_internal_randen_hwaes_impl.lib",
                "absl_random_internal_randen_slow.lib",
                "absl_random_internal_seed_material.lib",
                "absl_random_seed_gen_exception.lib",
                "absl_random_seed_sequences.lib",
                "absl_raw_hash_set.lib",
                "absl_raw_logging_internal.lib",
                "absl_scoped_set_env.lib",
                "absl_spinlock_wait.lib",
                "absl_stacktrace.lib",
                "absl_status.lib",
                "absl_statusor.lib",
                "absl_str_format_internal.lib",
                "absl_strerror.lib",
                "absl_string_view.lib",
                "absl_strings.lib",
                "absl_strings_internal.lib",
                "absl_symbolize.lib",
                "absl_synchronization.lib",
                "absl_throw_delegate.lib",
                "absl_time.lib",
                "absl_time_zone.lib",
                "absl_tracing_internal.lib",
                "absl_utf8_for_code_point.lib",
                "absl_vlog_config_internal.lib",
                "libprotobuf-lite.lib",
                "libupb.lib",
                "libutf8_range.lib",
                "libutf8_validity.lib",
            };

            foreach (string libraryName in protobufDependencyLibraries)
            {
                string libraryPath = Path.Combine(protobufLibDirectory, libraryName);
                if (!File.Exists(libraryPath))
                {
                    throw new BuildException($"CefProtobuf: protobuf dependency library not found: {libraryPath}");
                }

                PublicAdditionalLibraries.Add(libraryPath);
            }
        }
    }
}
