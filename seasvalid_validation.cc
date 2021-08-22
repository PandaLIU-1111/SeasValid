#include "seasvalid_validation.h"

using namespace std;


ZEND_BEGIN_ARG_INFO_EX(arginfo_seasvalid_validation_execute, 0, 0, 1)
    ZEND_ARG_INFO(0, params)
    ZEND_ARG_INFO(0, rules)
    ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

PHP_METHOD(seasvalid_validation, execute)
{
    zval result;

    zval* params = NULL;
    zval* rules = NULL;
    zval* message = NULL;

// init params and get params,rules
#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &params, &rules) == FAILURE)
    {
        return;
    }
#else
#undef IS_UNDEF
#define IS_UNDEF Z_EXPECTED_LONG
    ZEND_PARSE_PARAMETERS_START(1, 2)
    Z_PARAM_ARRAY(params)
    Z_PARAM_OPTIONAL
    Z_PARAM_ARRAY(rules)
    Z_PARAM_ARRAY(message)
    ZEND_PARSE_PARAMETERS_END();
#undef IS_UNDEF
#define IS_UNDEF 0
#endif

    HashTable *params_ht = Z_ARRVAL_P(params);
    HashTable *rules_ht = Z_ARRVAL_P(rules);
    HashTable *message_ht = Z_ARRVAL_P(message);

    zend_array *ret = zend_new_array(zend_array_count(rules_ht));
    zend_ulong idx;
    zend_string *key;
    zval *val;

    ZEND_HASH_FOREACH_KEY_VAL(rules_ht, idx, key, val) {
        convert_to_string(val);
        php_printf("key:%s  value:%s\n", key->val, Z_STRVAL_P(val));
        
    } ZEND_HASH_FOREACH_END();


    *return_value = result;
}

vector<string> split(const string& str, const string& delim)
{
    vector<string> res;
    if ("" == str) return res;

    char * strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());
    
    char * d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char * p = strtok(strs, d);
    while (p)
    {
        string s = p;
        res.push_back(s);
        p = strtok(NULL, d);   
    }
    
    return res;
}



const zend_function_entry seasvalid_validation_methods[] = 
{
    PHP_ME(seasvalid_validation, execute, arginfo_seasvalid_validation_execute, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/**
 * Define zend class entry
 */
zend_class_entry seasvalid_validation_ce;
zend_class_entry *seasvalid_validation_ce_ptr;

void seasvalid_validation_init()
{
    INIT_NS_CLASS_ENTRY(seasvalid_validation_ce, "Seasvalid", "Validation", seasvalid_validation_methods);
    seasvalid_validation_ce_ptr = zend_register_internal_class(&seasvalid_validation_ce TSRMLS_CC); // Registered in the Zend Engine
}