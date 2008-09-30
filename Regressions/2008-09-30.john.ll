; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = 
"e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:128:128"
target triple = "i386-apple-darwin9"
       %struct.ap_conf_vector_t = type opaque
       %struct.ap_configfile_t = type { i32 (i8*)*, i8* (i8*, i32, i8*)*, i32 (i8*)*, i8*, i8*, 
i32 }
       %struct.ap_directive_t = type { i8*, i8*, %struct.ap_directive_t*, 
%struct.ap_directive_t*, %struct.ap_directive_t*, i8*, i8*, i32 }
       %struct.ap_filter_func = type { i32 (%struct.ap_filter_t*, %struct.apr_bucket_brigade*)* 
}
       %struct.ap_filter_provider_t = type { i32, i32, %struct.ap_list_provider_names_t, 
%struct.ap_filter_rec_t*, %struct.ap_filter_provider_t*, i32, i8* }
       %struct.ap_filter_rec_t = type { i8*, %struct.ap_filter_func, i32 
(%struct.ap_filter_t*)*, i32, %struct.ap_filter_rec_t*, %struct.ap_filter_provider_t*, i32, i32 
}
       %struct.ap_filter_t = type { %struct.ap_filter_rec_t*, i8*, %struct.ap_filter_t*, 
%struct.request_rec*, %struct.conn_rec* }
       %struct.ap_list_provider_names_t = type { i8* }
       %struct.ap_method_list_t = type { i64, %struct.apr_array_header_t* }
       %struct.apr_array_header_t = type { %struct.apr_pool_t*, i32, i32, i32, i8* }
       %struct.apr_bucket = type { %struct.apr_bucket_list, %struct.apr_bucket_type_t*, i32, 
i64, i8*, void (i8*)*, %struct.apr_bucket_alloc_t* }
       %struct.apr_bucket_alloc_t = type opaque
       %struct.apr_bucket_brigade = type { %struct.apr_pool_t*, %struct.apr_bucket_list, 
%struct.apr_bucket_alloc_t* }
       %struct.apr_bucket_list = type { %struct.apr_bucket*, %struct.apr_bucket* }
       %struct.apr_bucket_type_t = type { i8*, i32, i32, void (i8*)*, i32 (%struct.apr_bucket*, 
i8**, i32*, i32)*, i32 (%struct.apr_bucket*, %struct.apr_pool_t*)*, i32 (%struct.apr_bucket*, 
i32)*, i32 (%struct.apr_bucket*, %struct.apr_bucket**)* }
       %struct.apr_descriptor = type { %struct.apr_file_t* }
       %struct.apr_file_t = type opaque
       %struct.apr_finfo_t = type { %struct.apr_pool_t*, i32, i32, i32, i32, i32, i32, i32, i32, 
i64, i64, i64, i64, i64, i8*, i8*, %struct.apr_file_t* }
       %struct.apr_pollfd_t = type { %struct.apr_pool_t*, i32, i16, i16, %struct.apr_descriptor, 
i8* }
       %struct.apr_pool_t = type opaque
       %struct.apr_sockaddr_t = type { %struct.apr_pool_t*, i8*, i8*, i16, i32, i32, i32, i32, 
i8*, %struct.apr_sockaddr_t*, { %struct.sockaddr_storage } }
       %struct.apr_table_t = type opaque
       %struct.apr_uri_t = type { i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct.hostent*, 
i16, i8 }
       %struct.cmd_func = type { i8* (%struct.cmd_parms*, i8*)* }
       %struct.cmd_parms = type { i8*, i32, i64, %struct.apr_array_header_t*, 
%struct.ap_method_list_t*, %struct.ap_configfile_t*, %struct.ap_directive_t*, 
%struct.apr_pool_t*, %struct.apr_pool_t*, %struct.server_rec*, i8*, %struct.command_rec*, 
%struct.ap_conf_vector_t*, %struct.ap_directive_t*, i32 }
       %struct.command_rec = type { i8*, %struct.cmd_func, i8*, i32, i32, i8* }
       %struct.conn_rec = type { %struct.apr_pool_t*, %struct.server_rec*, i8*, 
%struct.apr_sockaddr_t*, %struct.apr_sockaddr_t*, i8*, i8*, i8*, i8, i32, i8, i32, i8*, i8*, 
i32, %struct.ap_conf_vector_t*, %struct.apr_table_t*, %struct.ap_filter_t*, 
%struct.ap_filter_t*, i8*, %struct.apr_bucket_alloc_t*, %struct.conn_state_t*, i32, i32 }
       %struct.conn_state_t = type { { %struct.conn_state_t*, %struct.conn_state_t* }, i64, i32, 
%struct.conn_rec*, %struct.apr_pool_t*, %struct.apr_bucket_alloc_t*, %struct.apr_pollfd_t }
       %struct.hostent = type { i8*, i8**, i32, i32, i8** }
       %struct.htaccess_result = type { i8*, i32, i32, %struct.ap_conf_vector_t*, 
%struct.htaccess_result* }
       %struct.module = type { i32, i32, i32, i8*, i8*, %struct.module*, i32, void 
(%struct.process_rec*)*, i8* (%struct.apr_pool_t*, i8*)*, i8* (%struct.apr_pool_t*, i8*, i8*)*, 
i8* (%struct.apr_pool_t*, %struct.server_rec*)*, i8* (%struct.apr_pool_t*, i8*, i8*)*, 
%struct.command_rec*, void (%struct.apr_pool_t*)* }
       %struct.process_rec = type { %struct.apr_pool_t*, %struct.apr_pool_t*, i32, i8**, i8* }
       %struct.request_rec = type { %struct.apr_pool_t*, %struct.conn_rec*, %struct.server_rec*, 
%struct.request_rec*, %struct.request_rec*, %struct.request_rec*, i8*, i32, i32, i32, i8*, i32, 
i8*, i64, i8*, i32, i8*, i32, i64, %struct.apr_array_header_t*, %struct.ap_method_list_t*, i64, 
i64, i64, i32, i8*, i64, i64, i64, i32, i32, i32, %struct.apr_table_t*, %struct.apr_table_t*, 
%struct.apr_table_t*, %struct.apr_table_t*, %struct.apr_table_t*, i8*, i8*, i8*, 
%struct.apr_array_header_t*, i8*, i8*, i8*, i32, i32, i8*, i8*, i8*, i8*, i8*, i8*, 
%struct.apr_finfo_t, %struct.apr_uri_t, i32, %struct.ap_conf_vector_t*, 
%struct.ap_conf_vector_t*, %struct.htaccess_result*, %struct.ap_filter_t*, %struct.ap_filter_t*, 
%struct.ap_filter_t*, %struct.ap_filter_t*, i32 }
       %struct.server_addr_rec = type { %struct.server_addr_rec*, %struct.apr_sockaddr_t*, i16, 
i8* }
       %struct.server_rec = type { %struct.process_rec*, %struct.server_rec*, i8*, i32, i8*, 
i8*, i16, i8*, %struct.apr_file_t*, i32, i32, %struct.ap_conf_vector_t*, 
%struct.ap_conf_vector_t*, %struct.server_addr_rec*, i64, i64, i32, i32, i8*, i32, 
%struct.apr_array_header_t*, %struct.apr_array_header_t*, i32, i32, i32, i8* }
       %struct.sockaddr_storage = type { i8, i8, [6 x i8], i64, [112 x i8] }

define i32 @ap_parse_htaccess(%struct.ap_conf_vector_t** %result, %struct.request_rec* %r, i32 %override, i32 %override_opts, i8* %d, i8* %access_name) nounwind {
entry:
       %f = alloca %struct.ap_configfile_t*            ; <%struct.ap_configfile_t**> [#uses=2]
       br label %bb23

bb11:           ; preds = %bb23
       %0 = load %struct.ap_configfile_t** %f, align 4         ; <%struct.ap_configfile_t*> [#uses=1]
       %1 = getelementptr %struct.ap_configfile_t* %0, i32 0, i32 2            ; <i32 (i8*)**> [#uses=1]
       %2 = load i32 (i8*)** %1, align 4               ; <i32 (i8*)*> [#uses=1]
       %3 = call i32 %2(i8* null) nounwind            ; <i32> [#uses=0]
       unreachable

bb22:           ; preds = %bb23
       ret i32 403

bb23:           ; preds = %bb23, %bb23, %entry
       %4 = call i32 @ap_pcfg_openfile(%struct.ap_configfile_t** %f, 
%struct.apr_pool_t* null, i8* null) nounwind             ; <i32> [#uses=1]
       switch i32 %4, label %bb22 [
               i32 0, label %bb11
               i32 2, label %bb23
               i32 20, label %bb23
       ]
}

define void @ap_single_module_configure(%struct.apr_pool_t* %p, %struct.server_rec* %s, 
%struct.module* %m) nounwind {
entry:
       unreachable
}

define i32 @ap_open_logs(%struct.apr_pool_t* %pconf, %struct.apr_pool_t* %p, 
%struct.apr_pool_t* %ptemp, %struct.server_rec* %s_main) nounwind {
entry:
       unreachable
}

define void @ap_log_error(i8* %file, i32 %line, i32 %level, i32 %status, 
%struct.server_rec* %s, i8* %fmt, ...) nounwind {
entry:
       ret void
}

define void @ap_log_rerror(i8* %file, i32 %line, i32 %level, i32 %status, 
%struct.request_rec* %r, i8* %fmt, ...) nounwind {
entry:
       unreachable
}

define i32 @cfg_close(i8* %param) nounwind {
entry:
       ret i32 0
}

define i32 @ap_pcfg_openfile(%struct.ap_configfile_t** %ret_cfg, %struct.apr_pool_t* 
%p, i8* %name) nounwind {
entry:
       br i1 false, label %bb12, label %bb13

bb12:           ; preds = %entry
       %0 = bitcast i8* null to %struct.ap_configfile_t*               ; <%struct.ap_configfile_t*> [#uses=2]
       %1 = getelementptr %struct.ap_configfile_t* %0, i32 0, i32 2            ; <i32 (i8*)**> [#uses=1]
       store i32 (i8*)* @cfg_close, i32 (i8*)** %1, align 4
       store %struct.ap_configfile_t* %0, %struct.ap_configfile_t** %ret_cfg, align 4
       ret i32 0

bb13:           ; preds = %entry
       ret i32 0
}

define i8* @ap_escape_shell_cmd(%struct.apr_pool_t* %p, i8* %str) nounwind {
entry:
       unreachable
}

