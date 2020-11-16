/*
  +----------------------------------------------------------------------+
  | SeasLog                                                              |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Chitao Gao  <neeke@php.net>                                  |
  +----------------------------------------------------------------------+
*/

#include "php_seasValid.h"

ZEND_DECLARE_MODULE_GLOBALS(seaslog);
static PHP_GINIT_FUNCTION(seaslog);
static PHP_GSHUTDOWN_FUNCTION(seaslog);

#ifdef COMPILE_DL_SEASLOG
ZEND_GET_MODULE(seaslog)
#endif

#include "ErrorHook.h"
#include "Buffer.h"
#include "Datetime.h"
#include "ExceptionHook.h"
#include "Logger.h"
#include "Request.h"
#include "TemplateFormatter.h"
#include "StreamWrapper.h"
#include "Appender.h"
#include "Analyzer.h"
#include "Performance.h"
#include "Common.h"
#include "php7_wrapper.h"

zend_class_entry *seaslog_ce;

const zend_function_entry seaslog_functions[] =
{
    PHP_FE(seaslog_get_author,  NULL)
    {
        NULL, NULL, NULL
    }
};

#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep seaslog_deps[] =
{
    {
        NULL, NULL, NULL
    }
};
#endif

ZEND_BEGIN_ARG_INFO_EX(seaslog_setBasePath_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, base_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_setRequestID_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, request_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_setLogger_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, logger)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_closeLoggerStream_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, model)
ZEND_ARG_INFO(0, logger)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_setDatetimeFormat_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, format)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_analyzerCount_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, level)
ZEND_ARG_INFO(0, log_path)
ZEND_ARG_INFO(0, key_word)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_analyzerDetail_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, level)
ZEND_ARG_INFO(0, log_path)
ZEND_ARG_INFO(0, key_word)
ZEND_ARG_INFO(0, start)
ZEND_ARG_INFO(0, limit)
ZEND_ARG_INFO(0, order)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_log_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, level)
ZEND_ARG_INFO(0, message)
ZEND_ARG_INFO(0, context)
ZEND_ARG_INFO(0, logger)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_log_common_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, message)
ZEND_ARG_INFO(0, context)
ZEND_ARG_INFO(0, logger)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_setRequestVariable_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_getRequestVariable_arginfo, 0, 0, 1)
ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(seaslog_flushbuffer_arginfo, 0, 0, 0)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

const zend_function_entry seaslog_methods[] =
{
    PHP_ME(SEASLOG_RES_NAME, __construct,   NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(SEASLOG_RES_NAME, __destruct,    NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)

    PHP_ME(SEASLOG_RES_NAME, setBasePath,   seaslog_setBasePath_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getBasePath,   NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, setLogger,     seaslog_setLogger_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, closeLoggerStream,     seaslog_closeLoggerStream_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getLastLogger, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_ME(SEASLOG_RES_NAME, setRequestID,  seaslog_setRequestID_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getRequestID,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_ME(SEASLOG_RES_NAME, setDatetimeFormat,     seaslog_setDatetimeFormat_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getDatetimeFormat,     NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_ME(SEASLOG_RES_NAME, setRequestVariable,   seaslog_setRequestVariable_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getRequestVariable,   seaslog_getRequestVariable_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_ME(SEASLOG_RES_NAME, analyzerCount, seaslog_analyzerCount_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, analyzerDetail,seaslog_analyzerDetail_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getBuffer,     NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getBufferCount, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, getBufferEnabled,     NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, flushBuffer,   seaslog_flushbuffer_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_ME(SEASLOG_RES_NAME, log,           seaslog_log_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, debug,         seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, info,          seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, notice,        seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, warning,       seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, error,         seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, critical,      seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, alert,         seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(SEASLOG_RES_NAME, emergency,     seaslog_log_common_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    {
        NULL, NULL, NULL
    }
};

PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("seaslog.default_basepath", "/var/log/www", PHP_INI_SYSTEM, OnUpdateString, default_basepath, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.default_logger", "default", PHP_INI_SYSTEM, OnUpdateString, default_logger, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.default_datetime_format", "Y-m-d H:i:s", PHP_INI_SYSTEM, OnUpdateString, default_datetime_format, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.default_template", "%T | %L | %P | %Q | %t | %M", PHP_INI_SYSTEM, OnUpdateString, default_template, zend_seaslog_globals, seaslog_globals)


STD_PHP_INI_BOOLEAN("seaslog.disting_folder", "1", PHP_INI_SYSTEM, OnUpdateBool, disting_folder, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.disting_type", "0", PHP_INI_SYSTEM, OnUpdateBool, disting_type, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.disting_by_hour", "0", PHP_INI_SYSTEM, OnUpdateBool, disting_by_hour, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.use_buffer", "0", PHP_INI_SYSTEM, OnUpdateBool, use_buffer, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.buffer_size", "0", PHP_INI_ALL, OnUpdateLongGEZero, buffer_size, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.buffer_disabled_in_cli", "0", PHP_INI_SYSTEM, OnUpdateBool, buffer_disabled_in_cli, zend_seaslog_globals, seaslog_globals)


STD_PHP_INI_BOOLEAN("seaslog.trace_notice", "0", PHP_INI_ALL, OnUpdateBool, trace_notice, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.trace_warning", "0", PHP_INI_ALL, OnUpdateBool, trace_warning, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.trace_error", "1", PHP_INI_ALL, OnUpdateBool, trace_error, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_BOOLEAN("seaslog.trace_exception", "0", PHP_INI_SYSTEM, OnUpdateBool, trace_exception, zend_seaslog_globals, seaslog_globals)


STD_PHP_INI_ENTRY("seaslog.level", "8", PHP_INI_ALL, OnUpdateLongGEZero, level, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.recall_depth", "0", PHP_INI_ALL, OnUpdateLongGEZero, recall_depth, zend_seaslog_globals, seaslog_globals)


STD_PHP_INI_ENTRY("seaslog.appender", "1", PHP_INI_SYSTEM, OnUpdateLongGEZero, appender, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.appender_retry", "0", PHP_INI_ALL, OnUpdateLongGEZero, appender_retry, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.remote_host", "127.0.0.1", PHP_INI_ALL, OnUpdateString, remote_host, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.remote_port", "514", PHP_INI_ALL, OnUpdateLongGEZero, remote_port, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.remote_timeout", "1", PHP_INI_SYSTEM, OnUpdateLongGEZero, remote_timeout, zend_seaslog_globals, seaslog_globals)

STD_PHP_INI_BOOLEAN("seaslog.trim_wrap", "0", PHP_INI_ALL, OnUpdateBool, trim_wrap, zend_seaslog_globals, seaslog_globals)

STD_PHP_INI_BOOLEAN("seaslog.throw_exception", "1", PHP_INI_ALL, OnUpdateBool, throw_exception, zend_seaslog_globals, seaslog_globals)

STD_PHP_INI_BOOLEAN("seaslog.ignore_warning", "1", PHP_INI_ALL, OnUpdateBool, ignore_warning, zend_seaslog_globals, seaslog_globals)

STD_PHP_INI_BOOLEAN("seaslog.trace_performance", "0", PHP_INI_SYSTEM, OnUpdateBool, trace_performance, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_sample_rate", "10", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_sample_rate, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_start_depth", "1", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_start_depth, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_max_depth", "5", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_max_depth, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_max_functions_per_depth", "5", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_max_functions_per_depth, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_min_wall_time", "1000", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_min_wall_time, zend_seaslog_globals, seaslog_globals)
STD_PHP_INI_ENTRY("seaslog.trace_performance_min_function_wall_time", "10", PHP_INI_ALL, OnUpdateLongGEZero, trace_performance_min_function_wall_time, zend_seaslog_globals, seaslog_globals)

PHP_INI_END()

static PHP_GINIT_FUNCTION(seaslog)
{
    memset(seaslog_globals, 0, sizeof(zend_seaslog_globals));
    seaslog_globals->frame_free_list = NULL;
    seaslog_globals->performance_frames = NULL;
}

static PHP_GSHUTDOWN_FUNCTION(seaslog)
{

}

PHP_MINIT_FUNCTION(seaslog)
{
    zend_class_entry seaslog;

    REGISTER_INI_ENTRIES();

    REGISTER_STRINGL_CONSTANT("SEASLOG_VERSION",   SEASLOG_VERSION,   sizeof(SEASLOG_VERSION) - 1,  CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_AUTHOR",    SEASLOG_AUTHOR,    sizeof(SEASLOG_AUTHOR) - 1,   CONST_PERSISTENT | CONST_CS);

    REGISTER_STRINGL_CONSTANT("SEASLOG_ALL",       SEASLOG_ALL,       sizeof(SEASLOG_ALL) - 1,      CONST_PERSISTENT | CONST_CS);

    REGISTER_STRINGL_CONSTANT("SEASLOG_DEBUG",     SEASLOG_DEBUG,     sizeof(SEASLOG_DEBUG) - 1,    CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_INFO",      SEASLOG_INFO,      sizeof(SEASLOG_INFO) - 1,     CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_NOTICE",    SEASLOG_NOTICE,    sizeof(SEASLOG_NOTICE) - 1,   CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_WARNING",   SEASLOG_WARNING,   sizeof(SEASLOG_WARNING) - 1,  CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_ERROR",     SEASLOG_ERROR,     sizeof(SEASLOG_ERROR) - 1,    CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_CRITICAL",  SEASLOG_CRITICAL,  sizeof(SEASLOG_CRITICAL) - 1, CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_ALERT",     SEASLOG_ALERT,     sizeof(SEASLOG_ALERT) - 1,    CONST_PERSISTENT | CONST_CS);
    REGISTER_STRINGL_CONSTANT("SEASLOG_EMERGENCY", SEASLOG_EMERGENCY, sizeof(SEASLOG_EMERGENCY) - 1,CONST_PERSISTENT | CONST_CS);

    REGISTER_LONG_CONSTANT("SEASLOG_DETAIL_ORDER_ASC", SEASLOG_DETAIL_ORDER_ASC, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_DETAIL_ORDER_DESC", SEASLOG_DETAIL_ORDER_DESC, CONST_PERSISTENT | CONST_CS);

    REGISTER_LONG_CONSTANT("SEASLOG_APPENDER_FILE", SEASLOG_APPENDER_FILE, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_APPENDER_TCP", SEASLOG_APPENDER_TCP, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_APPENDER_UDP", SEASLOG_APPENDER_UDP, CONST_PERSISTENT | CONST_CS);

    REGISTER_LONG_CONSTANT("SEASLOG_CLOSE_LOGGER_STREAM_MOD_ALL", SEASLOG_CLOSE_LOGGER_STREAM_MOD_ALL, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_CLOSE_LOGGER_STREAM_MOD_ASSIGN", SEASLOG_CLOSE_LOGGER_STREAM_MOD_ASSIGN, CONST_PERSISTENT | CONST_CS);

    REGISTER_LONG_CONSTANT("SEASLOG_REQUEST_VARIABLE_DOMAIN_PORT", SEASLOG_REQUEST_VARIABLE_DOMAIN_PORT, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_REQUEST_VARIABLE_REQUEST_URI", SEASLOG_REQUEST_VARIABLE_REQUEST_URI, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_REQUEST_VARIABLE_REQUEST_METHOD", SEASLOG_REQUEST_VARIABLE_REQUEST_METHOD, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("SEASLOG_REQUEST_VARIABLE_CLIENT_IP", SEASLOG_REQUEST_VARIABLE_CLIENT_IP, CONST_PERSISTENT | CONST_CS);

    INIT_CLASS_ENTRY(seaslog, SEASLOG_RES_NAME, seaslog_methods);

#if PHP_VERSION_ID >= 70000
    seaslog_ce = zend_register_internal_class_ex(&seaslog, NULL);
#else
    seaslog_ce = zend_register_internal_class_ex(&seaslog, NULL, NULL TSRMLS_CC);
#endif

    seaslog_ce->ce_flags |= ZEND_ACC_FINAL;

    init_error_hooks(TSRMLS_C);
    init_exception_hooks(TSRMLS_C);
    init_buffer_switch(TSRMLS_C);
    init_remote_timeout(TSRMLS_C);
    init_zend_hooks(TSRMLS_C);

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(seaslog)
{
    recovery_error_hooks(TSRMLS_C);
    recovery_exception_hooks(TSRMLS_C);
    recovery_zend_hooks(TSRMLS_C);

    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}

PHP_RINIT_FUNCTION(seaslog)
{
    SEASLOG_G(initRComplete) = SEASLOG_INITR_COMPLETE_NO;
    SEASLOG_G(error_loop) = 0;

    seaslog_init_slash_or_underline(TSRMLS_C);
    seaslog_init_pid(TSRMLS_C);
    seaslog_init_host_name(TSRMLS_C);
    seaslog_init_request_id(TSRMLS_C);
    seaslog_init_auto_globals(TSRMLS_C);
    seaslog_init_request_variable(TSRMLS_C);
    seaslog_init_last_time(TSRMLS_C);
    seaslog_init_template(TSRMLS_C);
    seaslog_init_logger_list(TSRMLS_C);
    seaslog_init_logger(TSRMLS_C);
    seaslog_init_buffer(TSRMLS_C);
    seaslog_init_stream_list(TSRMLS_C);

    seaslog_rinit_performance(TSRMLS_C);

    SEASLOG_G(initRComplete) = SEASLOG_INITR_COMPLETE_YES;
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(seaslog)
{
    seaslog_clear_performance(seaslog_ce TSRMLS_CC);
    seaslog_shutdown_buffer(SEASLOG_BUFFER_RE_INIT_NO TSRMLS_CC);
    seaslog_clear_buffer(TSRMLS_C);
    seaslog_clear_logger(TSRMLS_C);
    seaslog_clear_logger_list(TSRMLS_C);
    seaslog_clear_last_time(TSRMLS_C);
    seaslog_clear_request_id(TSRMLS_C);
    seaslog_clear_pid(TSRMLS_C);
    seaslog_clear_host_name(TSRMLS_C);
    seaslog_clear_template(TSRMLS_C);
    seaslog_clear_request_variable(TSRMLS_C);
    seaslog_clear_stream(SEASLOG_STREAM_LIST_DESTROY_YES, SEASLOG_CLOSE_LOGGER_STREAM_MOD_ALL, NULL TSRMLS_CC);
    return SUCCESS;
}

PHP_MINFO_FUNCTION(seaslog)
{
    php_info_print_table_start();

    if (PG(expose_php) && !sapi_module.phpinfo_as_text)
    {
        php_info_print_table_header(2, "SeasLog support", SEASLOG_LOGO_IMG"enabled");
    }
    else
    {
        php_info_print_table_header(2, "SeasLog support", "Enabled");
    }

    php_info_print_table_row(2, "SeasLog Version", SEASLOG_VERSION);
    php_info_print_table_row(2, "SeasLog Author", SEASLOG_AUTHOR);
    php_info_print_table_row(2, "SeasLog Supports", SEASLOG_SUPPORTS);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

zend_module_entry seaslog_module_entry =
{
#if ZEND_MODULE_API_NO >= 20050922
    STANDARD_MODULE_HEADER_EX, NULL,
    seaslog_deps,
#else
    STANDARD_MODULE_HEADER,
#endif
    SEASLOG_RES_NAME,
    seaslog_functions,
    PHP_MINIT(seaslog),
    PHP_MSHUTDOWN(seaslog),
    PHP_RINIT(seaslog),
    PHP_RSHUTDOWN(seaslog),
    PHP_MINFO(seaslog),
    SEASLOG_VERSION,
    PHP_MODULE_GLOBALS(seaslog),
    PHP_GINIT(seaslog),
    PHP_GSHUTDOWN(seaslog),
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

static inline int seaslog_log_context_ex(int argc, int check_argc, char *level, int level_int, char *message, int message_len, zval *context, char *module, int module_len, zend_class_entry *seaslog_ce TSRMLS_DC)
{
    if (argc > check_argc)
    {
        if (seaslog_log_context(argc, level, level_int, message, message_len, HASH_OF(context), module, module_len, seaslog_ce TSRMLS_CC) == FAILURE)
        {
            return FAILURE;
        }
    }
    else
    {
        if (seaslog_log_ex(argc, level, level_int, message, message_len, "", 0, seaslog_ce TSRMLS_CC) == FAILURE)
        {
            return FAILURE;
        }
    }

    return SUCCESS;
}

static inline int seaslog_log_by_level_common_ex(int argc, int check_argc, char *level, int level_int, zval *messages, zval *context, char *logger_str, int logger_len, zend_class_entry *seaslog_ce TSRMLS_DC)
{
    HashTable *msght;
    zval *pzval;
    zval **ppzval;

#if PHP_VERSION_ID >= 70000
    zend_ulong num_key;
    zend_string *str_key;

    switch (Z_TYPE_P(messages))
    {
    case IS_ARRAY:
    {
        msght = Z_ARRVAL_P(messages);
        ZEND_HASH_FOREACH_KEY_VAL(msght, num_key, str_key, pzval)
        {
            zend_string *s = zval_get_string(pzval);
            if (FAILURE == seaslog_log_context_ex(argc, check_argc, level, level_int, ZSTR_VAL(s), ZSTR_LEN(s), context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
            {
                return FAILURE;
            }
            zend_string_release(s);
        }
        ZEND_HASH_FOREACH_END();
    }
    break;
    case IS_STRING:
    default:
    {
        zend_string *s = zval_get_string(messages);
        if (FAILURE == seaslog_log_context_ex(argc, check_argc, level, level_int, ZSTR_VAL(s), ZSTR_LEN(s), context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
        {
            zend_string_release(s);
            return FAILURE;
        }
        zend_string_release(s);
    }
    }

#else

    switch (Z_TYPE_P(messages))
    {
    case IS_ARRAY:
    {
        msght = Z_ARRVAL_P(messages);

        zend_hash_internal_pointer_reset(msght);
        while (zend_hash_get_current_data(msght, (void **)&ppzval) == SUCCESS)
        {
            convert_to_string_ex(ppzval);
            if (FAILURE == seaslog_log_context_ex(argc, check_argc, level, level_int, Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval), context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
            {
                return FAILURE;
            }
            zend_hash_move_forward(msght);
        }
    }
    break;
    case IS_STRING:
    default:
        convert_to_string_ex(&messages);
        if (FAILURE == seaslog_log_context_ex(argc, check_argc, level, level_int, Z_STRVAL_P(messages), Z_STRLEN_P(messages), context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
        {
            return FAILURE;
        }
    }

#endif

    return SUCCESS;
}

static inline int seaslog_log_by_level_common_check_context(int argc, int check_argc, zval *context TSRMLS_DC)
{
    if (argc > check_argc && IS_ARRAY != Z_TYPE_P(context))
    {
        switch(check_argc)
        {
        case SEASLOG_LOG_FUNCTION_ARGC_USUAL:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "The second argument is not an array");
            break;
        case SEASLOG_LOG_FUNCTION_ARGC_UNUSUAL:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "The three argument is not an array");
            break;
        }

        return FAILURE;
    }

    return SUCCESS;
}

static void seaslog_log_by_level_common(INTERNAL_FUNCTION_PARAMETERS, char *level, int level_int)
{
    int argc = ZEND_NUM_ARGS();

    zval *messages = NULL;
    zval *context = NULL;
    char *logger_str = "";
    int logger_len = 0;

#if PHP_VERSION_ID >= 70000
    zend_string *logger = NULL;

    if (zend_parse_parameters(argc TSRMLS_CC, "z|zS", &messages, &context, &logger) == FAILURE)
    {
        return;
    }

    if (FAILURE == seaslog_log_by_level_common_check_context(argc, SEASLOG_LOG_FUNCTION_ARGC_USUAL, context TSRMLS_CC))
    {
        RETURN_FALSE;
    }

    if (logger != NULL)
    {
        logger_str = ZSTR_VAL(logger);
        logger_len = ZSTR_LEN(logger);
    }

    if (FAILURE == seaslog_log_by_level_common_ex(argc, SEASLOG_LOG_FUNCTION_ARGC_USUAL, level, level_int, messages, context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
    {
        RETURN_FALSE;
    }

#else

    if (zend_parse_parameters(argc TSRMLS_CC, "z|zs", &messages, &context, &logger_str, &logger_len) == FAILURE)
    {
        return;
    }

    if (FAILURE == seaslog_log_by_level_common_check_context(argc, SEASLOG_LOG_FUNCTION_ARGC_USUAL, context TSRMLS_CC))
    {
        RETURN_FALSE;
    }

    if (FAILURE == seaslog_log_by_level_common_ex(argc, SEASLOG_LOG_FUNCTION_ARGC_USUAL, level, level_int, messages, context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
    {
        RETURN_FALSE;
    }

#endif

    RETURN_TRUE;
}

/* {{{ proto string seaslog_get_author()
   Return SeasLog author */
PHP_FUNCTION(seaslog_get_author)
{
    SEASLOG_RETURN_STRINGL(SEASLOG_AUTHOR, strlen(SEASLOG_AUTHOR));
}
/* }}} */

PHP_METHOD(SEASLOG_RES_NAME, __construct)
{
    RETURN_TRUE;
}

/* {{{ proto string getRequestID()
   Get SeasLog request_id differentiated requests */
PHP_METHOD(SEASLOG_RES_NAME, getRequestID)
{
    SEASLOG_RETURN_STRINGL(SEASLOG_G(request_id), strlen(SEASLOG_G(request_id)));
}
/* }}} */

/* {{{ proto array getBuffer()
   Get the logs buffer in memory as array */
PHP_METHOD(SEASLOG_RES_NAME, getBuffer)
{
    if (seaslog_check_buffer_enable(TSRMLS_C))
    {
#if PHP_VERSION_ID >= 70000
        RETURN_ZVAL(&SEASLOG_G(buffer), 1, 0);
#else
        RETURN_ZVAL(SEASLOG_G(buffer), 1, 0);
#endif
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool log(string level, string message [, array context, string logger])
   The Common Record Log Function  */
PHP_METHOD(SEASLOG_RES_NAME, log)
{
    int argc = ZEND_NUM_ARGS();
    int level_int;

    zval *messages = NULL;
    zval *context = NULL;
    char *logger_str = "";
    int logger_len = 0;
    HashTable *msght;
    zval *pzval;
    zval **ppzval;

#if PHP_VERSION_ID >= 70000
    zend_string *level;
    zend_string *logger = NULL;

    if (zend_parse_parameters(argc TSRMLS_CC, "Sz|zS", &level, &messages, &context, &logger) == FAILURE)
    {
        return;
    }

    if (FAILURE == seaslog_log_by_level_common_check_context(argc, SEASLOG_LOG_FUNCTION_ARGC_UNUSUAL, context TSRMLS_CC))
    {
        RETURN_FALSE;
    }

    level_int = seaslog_get_level_int(ZSTR_VAL(level));

    if (logger != NULL)
    {
        logger_str = ZSTR_VAL(logger);
        logger_len = ZSTR_LEN(logger);
    }

    if (FAILURE == seaslog_log_by_level_common_ex(argc, SEASLOG_LOG_FUNCTION_ARGC_UNUSUAL, ZSTR_VAL(level), level_int, messages, context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
    {
        RETURN_FALSE;
    }

#else
    char *level;
    int level_len = 0;

    if (zend_parse_parameters(argc TSRMLS_CC, "sz|zs", &level, &level_len, &messages, &context, &logger_str, &logger_len) == FAILURE)
    {
        return;
    }

    if (FAILURE == seaslog_log_by_level_common_check_context(argc, SEASLOG_LOG_FUNCTION_ARGC_UNUSUAL, context TSRMLS_CC))
    {
        RETURN_FALSE;
    }

    level_int = seaslog_get_level_int(level);

    if (FAILURE == seaslog_log_by_level_common_ex(argc, SEASLOG_LOG_FUNCTION_ARGC_UNUSUAL, level, level_int, messages, context, logger_str, logger_len, seaslog_ce TSRMLS_CC))
    {
        RETURN_FALSE;
    }

#endif


    RETURN_TRUE;
}
/* }}} */
