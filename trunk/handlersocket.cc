
extern "C" {
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#include "php_handlersocket.h"
}

#include <sstream>
#include <hstcpcli.hpp> /* handlersocket */


#define HS_PRIMARY "PRIMARY"

#define HS_PROTOCOL_OPEN    "P"
#define HS_PROTOCOL_AUTH    "A"
#define HS_PROTOCOL_INSERT  "+"
#define HS_PROTOCOL_FILTER  "F"
#define HS_PROTOCOL_WHILE   "W"
#define HS_PROTOCOL_IN      "@"

#define HS_FIND_EQUAL         "="
#define HS_FIND_LESS          "<"
#define HS_FIND_LESS_EQUAL    "<="
#define HS_FIND_GREATER       ">"
#define HS_FIND_GREATER_EQUAL ">="

#define HS_MODIFY_UPDATE         "U"
#define HS_MODIFY_INCREMENT      "+"
#define HS_MODIFY_DECREMENT      "-"
#define HS_MODIFY_REMOVE         "D"
#define HS_MODIFY_UPDATE_PREV    "U?"
#define HS_MODIFY_INCREMENT_PREV "+?"
#define HS_MODIFY_DECREMENT_PREV "-?"
#define HS_MODIFY_REMOVE_PREV    "D?"

static zend_class_entry *hs_ce = NULL;
static zend_class_entry *hs_index_ce = NULL;
static zend_class_entry *hs_exception_ce = NULL;

typedef struct
{
    zend_object object;
    zval *error;
    dena::hstcpcli_i *cli;
} php_hs_t;

typedef struct
{
    zend_object object;
    long id;
    zval *link;
    zval *filter;
    zval *error;
} php_hs_index_t;

#define PUSH_PARAM(arg) zend_vm_stack_push(arg TSRMLS_CC)
#define POP_PARAM() (void)zend_vm_stack_pop(TSRMLS_C)

#define HS_METHOD_BASE(classname, name) zim_##classname##_##name

#define HS_METHOD_HELPER(classname, name, retval, thisptr, num, param) \
  PUSH_PARAM(param); PUSH_PARAM((void*)num); \
  HS_METHOD_BASE(classname, name)(num, retval, NULL, thisptr, 0 TSRMLS_CC); \
  POP_PARAM(); POP_PARAM();

#define HS_METHOD(classname, name, retval, thisptr) \
  HS_METHOD_BASE(classname, name)(0, retval, NULL, thisptr, 0 TSRMLS_CC);

#define HS_METHOD6(classname, name, retval, thisptr, param1, param2, param3, param4, param5, param6) \
  PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3); PUSH_PARAM(param4);; PUSH_PARAM(param5); \
  HS_METHOD_HELPER(classname, name, retval, thisptr, 6, param6); \
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

#define HS_METHOD7(classname, name, retval, thisptr, param1, param2, param3, param4, param5, param6, param7) \
  PUSH_PARAM(param1); PUSH_PARAM(param2); PUSH_PARAM(param3); PUSH_PARAM(param4); PUSH_PARAM(param5); PUSH_PARAM(param6); \
  HS_METHOD_HELPER(classname, name, retval, thisptr, 7, param7); \
  POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM(); POP_PARAM();

#define HS_CHECK_OBJECT(member, classname) \
  if (!(member)) { \
    zend_throw_exception_ex(hs_exception_ce, 0 TSRMLS_CC, "The " #classname " object has not been correctly initialized by its constructor"); \
    RETURN_FALSE; \
  }

#define HS_INDEX_PROPERTY(property, retval) \
  retval = zend_read_property(hs_index_ce, getThis(), #property, strlen(#property), 0 TSRMLS_CC);

#define HS_ERROR_RESET(error) \
  if (error) { zval_ptr_dtor(&error); } MAKE_STD_ZVAL(error); ZVAL_NULL(error);


static zend_object_value hs_new(zend_class_entry *ce TSRMLS_DC);
static ZEND_METHOD(HandlerSocket, __construct);
static ZEND_METHOD(HandlerSocket, auth);
static ZEND_METHOD(HandlerSocket, openIndex);
static ZEND_METHOD(HandlerSocket, executeSingle);
static ZEND_METHOD(HandlerSocket, executeMulti);
static ZEND_METHOD(HandlerSocket, executeUpdate);
static ZEND_METHOD(HandlerSocket, executeDelete);
static ZEND_METHOD(HandlerSocket, executeInsert);
static ZEND_METHOD(HandlerSocket, getError);
static ZEND_METHOD(HandlerSocket, createIndex);

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs___construct, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_auth, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_openIndex, 0, 0, 5)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, db)
    ZEND_ARG_INFO(0, table)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, field)
    ZEND_ARG_INFO(0, filter)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_executeSingle, 0, 0, 3)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, operate)
    ZEND_ARG_INFO(0, criteria)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, update)
    ZEND_ARG_INFO(0, values)
    ZEND_ARG_INFO(0, filters)
    ZEND_ARG_INFO(0, in_key)
    ZEND_ARG_INFO(0, in_values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_executeMulti, 0, 0, 1)
    ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_executeUpdate, 0, 0, 4)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, operate)
    ZEND_ARG_INFO(0, criteria)
    ZEND_ARG_INFO(0, values)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, filters)
    ZEND_ARG_INFO(0, in_key)
    ZEND_ARG_INFO(0, in_values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_executeDelete, 0, 0, 3)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, operate)
    ZEND_ARG_INFO(0, criteria)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, filters)
    ZEND_ARG_INFO(0, in_key)
    ZEND_ARG_INFO(0, in_values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_executeInsert, 0, 0, 2)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, field)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_getError, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_createIndex, 0, 0, 5)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, db)
    ZEND_ARG_INFO(0, table)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, fields)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

static const zend_function_entry hs_methods[] = {
    ZEND_ME(HandlerSocket, __construct,
            arginfo_hs___construct, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, auth,
            arginfo_hs_auth, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, openIndex,
            arginfo_hs_openIndex, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, executeSingle,
            arginfo_hs_executeSingle, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, executeMulti,
            arginfo_hs_executeMulti, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, executeUpdate,
            arginfo_hs_executeUpdate, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, executeDelete,
            arginfo_hs_executeDelete, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, executeInsert,
            arginfo_hs_executeInsert, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, getError,
            arginfo_hs_getError, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocket, createIndex,
            arginfo_hs_createIndex, ZEND_ACC_PUBLIC)
    ZEND_MALIAS(HandlerSocket, executeFind, executeSingle,
                arginfo_hs_executeSingle, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


static zend_object_value hs_index_new(zend_class_entry *ce TSRMLS_DC);
static ZEND_METHOD(HandlerSocketIndex, __construct);
static ZEND_METHOD(HandlerSocketIndex, getId);
static ZEND_METHOD(HandlerSocketIndex, getDatabase);
static ZEND_METHOD(HandlerSocketIndex, getTable);
static ZEND_METHOD(HandlerSocketIndex, getName);
static ZEND_METHOD(HandlerSocketIndex, getField);
static ZEND_METHOD(HandlerSocketIndex, getFilter);
static ZEND_METHOD(HandlerSocketIndex, getOperator);
static ZEND_METHOD(HandlerSocketIndex, getError);
static ZEND_METHOD(HandlerSocketIndex, find);
static ZEND_METHOD(HandlerSocketIndex, insert);
static ZEND_METHOD(HandlerSocketIndex, update);
static ZEND_METHOD(HandlerSocketIndex, remove);
static ZEND_METHOD(HandlerSocketIndex, multi);

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index___construct, 0, 0, 6)
    ZEND_ARG_INFO(0, hs)
    ZEND_ARG_INFO(0, id)
    ZEND_ARG_INFO(0, db)
    ZEND_ARG_INFO(0, table)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, fields)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getId, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getDatabase, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getTable, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getName, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getField, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getFilter, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getOperator, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_getError, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_find, 0, 0, 1)
    ZEND_ARG_INFO(0, query)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_insert, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_update, 0, 0, 2)
    ZEND_ARG_INFO(0, query)
    ZEND_ARG_INFO(0, update)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_remove, 0, 0, 1)
    ZEND_ARG_INFO(0, query)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hs_index_multi, 0, 0, 1)
    ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

static const zend_function_entry hs_index_methods[] = {
    ZEND_ME(HandlerSocketIndex, __construct,
            arginfo_hs_index___construct, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getId,
            arginfo_hs_index_getId, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getDatabase,
            arginfo_hs_index_getDatabase, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getTable,
            arginfo_hs_index_getTable, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getName,
            arginfo_hs_index_getName, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getField,
            arginfo_hs_index_getField, ZEND_ACC_PUBLIC)
    ZEND_MALIAS(HandlerSocketIndex, getColumn, getField,
                arginfo_hs_index_getField, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getFilter,
            arginfo_hs_index_getFilter, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getOperator,
            arginfo_hs_index_getOperator, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, getError,
            arginfo_hs_index_getError, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, find,
            arginfo_hs_index_find, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, insert,
            arginfo_hs_index_insert, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, update,
            arginfo_hs_index_update, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, remove,
            arginfo_hs_index_remove, ZEND_ACC_PUBLIC)
    ZEND_ME(HandlerSocketIndex, multi,
            arginfo_hs_index_multi, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


PHP_MINIT_FUNCTION(handlersocket)
{
    zend_class_entry ce;

    /* HandlerSocket class */
    INIT_CLASS_ENTRY(
        ce, "HandlerSocket", (zend_function_entry *)hs_methods);
    hs_ce = zend_register_internal_class(&ce TSRMLS_CC);
    hs_ce->create_object = hs_new;

    /* constant */
#if ZEND_MODULE_API_NO < 20050922
    REGISTER_STRING_CONSTANT(
        "HANDLERSOCKET_PRIMARY", HS_PRIMARY, CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT(
        "HANDLERSOCKET_UPDATE", HS_MODIFY_UPDATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT(
        "HANDLERSOCKET_DELETE", HS_MODIFY_REMOVE, CONST_CS | CONST_PERSISTENT);
#else
    zend_declare_class_constant_string(
        hs_ce, "PRIMARY", strlen("PRIMARY"), HS_PRIMARY TSRMLS_CC);
    zend_declare_class_constant_string(
        hs_ce, "UPDATE", strlen("UPDATE"), HS_MODIFY_UPDATE TSRMLS_CC);
    zend_declare_class_constant_string(
        hs_ce, "DELETE", strlen("DELETE"), HS_MODIFY_REMOVE TSRMLS_CC);
#endif

    /* HandlerSocketIndex class */
    INIT_CLASS_ENTRY(
        ce, "HandlerSocketIndex", (zend_function_entry *)hs_index_methods);
    hs_index_ce = zend_register_internal_class(&ce TSRMLS_CC);
    hs_index_ce->create_object = hs_index_new;

    /* property */
    zend_declare_property_null(
        hs_index_ce, "_db", strlen("_db"),
        ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(
        hs_index_ce, "_table", strlen("_table"),
        ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(
        hs_index_ce, "_name", strlen("_name"),
        ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(
        hs_index_ce, "_field", strlen("_field"),
        ZEND_ACC_PROTECTED TSRMLS_CC);

    /* HandlerSocketException class */
    INIT_CLASS_ENTRY(ce, "HandlerSocketException", NULL);
    hs_exception_ce = zend_register_internal_class_ex(
        &ce, (zend_class_entry*)zend_exception_get_default(TSRMLS_C),
        NULL TSRMLS_CC);

    return SUCCESS;
}

/*
PHP_MSHUTDOWN_FUNCTION(handlersocket)
{
    return SUCCESS;
}

PHP_RINIT_FUNCTION(handlersocket)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(handlersocket)
{
    return SUCCESS;
}
*/

PHP_MINFO_FUNCTION(handlersocket)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "MySQL HandlerSocket support", "enabled");
    php_info_print_table_row(
        2, "extension Version", HANDLERSOCKET_EXTENSION_VERSION);
    php_info_print_table_row(2, "hsclient Library Support", "enabled");
    php_info_print_table_end();
}

zend_module_entry handlersocket_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "handlersocket",
    NULL,
    PHP_MINIT(handlersocket),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(handlersocket),
#if ZEND_MODULE_API_NO >= 20010901
    HANDLERSOCKET_EXTENSION_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_HANDLERSOCKET
ZEND_GET_MODULE(handlersocket)
#endif


inline static std::string hs_long_to_string(long n)
{
    std::ostringstream s;
    s << std::dec << n;
    return s.str();
}

inline static void hs_array_to_vector(
    zval *value, std::vector<dena::string_ref>& vec TSRMLS_DC)
{
    size_t n;
    zval **data;
    HashPosition pos;
    HashTable *ht;

    ht = HASH_OF(value);
    n = zend_hash_num_elements(ht);

    if (n == 0)
    {
        vec.push_back(dena::string_ref());
        return;
    }

    vec.reserve(n);

    zend_hash_internal_pointer_reset_ex(ht, &pos);
    while (zend_hash_get_current_data_ex(ht, (void **)&data, &pos) == SUCCESS)
    {
        if (Z_TYPE_PP(data) == IS_STRING)
        {
            vec.push_back(
                dena::string_ref(Z_STRVAL_PP(data), Z_STRLEN_PP(data)));
        }
        else if (Z_TYPE_PP(data) == IS_LONG ||
                 Z_TYPE_PP(data) == IS_DOUBLE ||
                 Z_TYPE_PP(data) == IS_BOOL)
        {
            convert_to_string(*data);
            vec.push_back(
                dena::string_ref(Z_STRVAL_PP(data), Z_STRLEN_PP(data)));
        }
        else
        {
            vec.push_back(dena::string_ref());
        }
        zend_hash_move_forward_ex(ht, &pos);
    }
}

inline static void hs_array_to_conf(HashTable *ht, dena::config& conf TSRMLS_DC)
{
    zval **tmp;
    HashPosition pos;
    char *key;
    uint key_len;
    ulong key_index;

    zend_hash_internal_pointer_reset_ex(ht, &pos);
    while (zend_hash_get_current_data_ex(ht, (void **)&tmp, &pos) == SUCCESS)
    {
        if (zend_hash_get_current_key_ex(
                ht, &key, &key_len, &key_index, 0, &pos)  == HASH_KEY_IS_STRING)
        {
            convert_to_string(*tmp);
            if (strcmp(key, "host") == 0)
            {
                conf["host"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "port") == 0)
            {
                conf["port"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "timeout") == 0)
            {
                conf["timeout"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "listen_backlog") == 0)
            {
                conf["listen_backlog"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "sndbuf") == 0)
            {
                conf["sndbuf"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "rcvbuf") == 0)
            {
                conf["rcvbuf"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "use_epoll") == 0)
            {
                conf["use_epoll"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "num_threads") == 0)
            {
                conf["num_threads"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "readsize") == 0)
            {
                conf["readsize"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "accept_balance") == 0)
            {
                conf["accept_balance"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "wrlock_timeout") == 0)
            {
                conf["wrlock_timeout"] = std::string(Z_STRVAL_PP(tmp));
            }
            else if (strcmp(key, "for_write") == 0)
            {
                conf["for_write"] = std::string(Z_STRVAL_PP(tmp));
            }
        }
        zend_hash_move_forward_ex(ht, &pos);
    }
}

static void hs_array_to_string(std::string& str, HashTable *ht TSRMLS_DC)
{
    zval **tmp;
    HashPosition pos;

    if (zend_hash_num_elements(ht) >= 0)
    {
        zend_hash_internal_pointer_reset_ex(ht, &pos);
        while (zend_hash_get_current_data_ex(
                   ht, (void **)&tmp, &pos) == SUCCESS)
        {
            switch ((*tmp)->type)
            {
                case IS_STRING:
                    str.append(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
                    break;
                default:
                    convert_to_string(*tmp);
                    str.append(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
                    break;
            }

            str.append(",", 1);

            zend_hash_move_forward_ex(ht, &pos);
        }

        str.erase(str.size() - 1, 1);
    }
}

static void hs_array_to_vector_filter(
    std::vector<dena::hstcpcli_filter>& vec, HashTable *ht TSRMLS_DC)
{
    zval **tmp;
    dena::hstcpcli_filter fe;

    if (zend_hash_index_find(ht, 0, (void **)&tmp) != SUCCESS)
    {
        return;
    }

    if (Z_TYPE_PP(tmp) == IS_ARRAY)
    {
        long n, i;

        hs_array_to_vector_filter(vec, HASH_OF(*tmp) TSRMLS_CC);

        n = zend_hash_num_elements(ht);
        for (i = n - 1; i >= 1; i--)
        {
            if (zend_hash_index_find(ht, i, (void **)&tmp) != SUCCESS ||
                Z_TYPE_PP(tmp) != IS_ARRAY)
            {
                continue;
            }

            hs_array_to_vector_filter(vec, HASH_OF(*tmp) TSRMLS_CC);
        }
    }
    else if (zend_hash_num_elements(ht) < 4)
    {
        return;
    }
    else
    {
        convert_to_string(*tmp);
        fe.filter_type = dena::string_ref(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));

        if (zend_hash_index_find(ht, 1, (void **)&tmp) == SUCCESS)
        {
            convert_to_string(*tmp);
            fe.op = dena::string_ref(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
        }

        if (zend_hash_index_find(ht, 2, (void **)&tmp) == SUCCESS)
        {
            convert_to_long(*tmp);
            fe.ff_offset = Z_LVAL_PP(tmp);
        }

        if (zend_hash_index_find(ht, 3, (void **)&tmp) == SUCCESS)
        {
            if (Z_TYPE_PP(tmp) == IS_STRING)
            {
                fe.val = dena::string_ref(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
            }
            else if (Z_TYPE_PP(tmp) == IS_LONG ||
                     Z_TYPE_PP(tmp) == IS_DOUBLE ||
                     Z_TYPE_PP(tmp) == IS_BOOL)
            {
                convert_to_string(*tmp);
                fe.val = dena::string_ref(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
            }
            else
            {
                fe.val = dena::string_ref();
            }
        }

        vec.push_back(fe);
    }
}

static zval* hs_zval_search_key(zval *value, zval *array TSRMLS_DC)
{
    zval *return_value, **entry, res;
    HashPosition pos;
    HashTable *ht;
    ulong index;
    uint key_len;
    char *key;
    int (*is_equal_func)(zval *, zval *, zval * TSRMLS_DC) = is_equal_function;

    MAKE_STD_ZVAL(return_value);

    if (array == NULL || Z_TYPE_P(array) != IS_ARRAY)
    {
        ZVAL_NULL(return_value);
        return return_value;
    }

    ht = HASH_OF(array);
    zend_hash_internal_pointer_reset_ex(ht, &pos);
    while (zend_hash_get_current_data_ex(ht, (void **)&entry, &pos) == SUCCESS)
    {
        is_equal_func(&res, value, *entry TSRMLS_CC);
        if (Z_LVAL(res))
        {
            switch (zend_hash_get_current_key_ex(
                        ht, &key, &key_len, &index, 0, &pos))
            {
                case HASH_KEY_IS_STRING:
                    ZVAL_STRINGL(return_value, key, key_len - 1, 1);
                    break;
                case HASH_KEY_IS_LONG:
                    ZVAL_LONG(return_value, index);
                    break;
                default:
                    ZVAL_NULL(return_value);
                    break;
            }

            return return_value;
        }
        zend_hash_move_forward_ex(ht, &pos);
    }

    ZVAL_NULL(return_value);

    return return_value;
}

static void hs_zval_to_filter(
    zval **return_value, zval *filter, zval *value, char *type TSRMLS_DC)
{
    HashTable *ht;
    HashPosition pos;
    zval **tmp, **ftmp, **vtmp, *index, *item;
    long n;

    if (value == NULL || Z_TYPE_P(value) != IS_ARRAY)
    {
        return;
    }

    ht = HASH_OF(value);
    n = zend_hash_num_elements(ht);

    if (n <= 0 ||
        zend_hash_index_find(ht, 0, (void **)&tmp) != SUCCESS)
    {
        return;
    }

    zend_hash_internal_pointer_reset_ex(ht, &pos);

    if (Z_TYPE_PP(tmp) == IS_ARRAY)
    {
        do
        {
            if (zend_hash_move_forward_ex(ht, &pos) < 0)
            {
                break;
            }

            hs_zval_to_filter(return_value, filter, *tmp, type TSRMLS_CC);
        }
        while (
            zend_hash_get_current_data_ex(ht, (void **)&tmp, &pos) == SUCCESS);

        return;
    }
    else if (n < 3)
    {
        return;
    }

    if (zend_hash_index_find(ht, 1, (void **)&ftmp) != SUCCESS)
    {
        return;
    }

    index = hs_zval_search_key(*ftmp, filter TSRMLS_CC);
    if (Z_TYPE_P(index) != IS_LONG)
    {
        zval_ptr_dtor(&index);
        return;
    }

    if (zend_hash_index_find(ht, 2, (void **)&vtmp) != SUCCESS)
    {
        zval_ptr_dtor(&index);
        return;
    }

    MAKE_STD_ZVAL(item);
    array_init(item);

    add_next_index_stringl(item, type, strlen(type), 1);

    convert_to_string(*tmp);
    add_next_index_stringl(item, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);

    add_next_index_long(item, Z_LVAL_P(index));

    if (Z_TYPE_PP(vtmp) == IS_NULL)
    {
        add_next_index_null(item);
    }
    else if (Z_TYPE_PP(vtmp) == IS_LONG)
    {
        add_next_index_long(item, Z_LVAL_PP(vtmp));
    }
    else if (Z_TYPE_PP(vtmp) == IS_DOUBLE)
    {
        add_next_index_double(item, Z_DVAL_PP(vtmp));
    }
    else
    {
        convert_to_string(*tmp);
        add_next_index_stringl(item, Z_STRVAL_PP(vtmp), Z_STRLEN_PP(vtmp), 1);
    }

    if (!(*return_value))
    {
        MAKE_STD_ZVAL(*return_value);
        array_init(*return_value);
    }

    add_next_index_zval(*return_value, item);

    zval_ptr_dtor(&index);
}

static void hs_array_to_in_filter(
    HashTable *ht, zval *filter,
    zval **filters, long *in_key, zval **in_values TSRMLS_DC)
{
    HashPosition pos;
    zval **val;

    char *key;
    ulong key_index;
    uint key_len;

    zend_hash_internal_pointer_reset_ex(ht, &pos);
    while (zend_hash_get_current_data_ex(ht, (void **)&val, &pos) == SUCCESS)
    {
        if (zend_hash_get_current_key_ex(
                ht, &key, &key_len, &key_index, 0, &pos) != HASH_KEY_IS_STRING)
        {
            zend_hash_move_forward_ex(ht, &pos);
            continue;
        }

        if (strcmp(key, "in") == 0)
        {
            /* in */
            if (Z_TYPE_PP(val) == IS_ARRAY)
            {
                HashTable *in_ht;
                HashPosition in_pos;
                zval **tmp;

                char *in_key_name;
                ulong in_key_index;
                uint in_key_len;

                in_ht = HASH_OF(*val);

                zend_hash_internal_pointer_reset_ex(in_ht, &in_pos);
                if (zend_hash_get_current_data_ex(
                        in_ht, (void **)&tmp, &in_pos) == SUCCESS)
                {
                    if (Z_TYPE_PP(tmp) == IS_ARRAY)
                    {
                        switch (zend_hash_get_current_key_ex(
                                    in_ht, &in_key_name, &in_key_len,
                                    &in_key_index, 0, &in_pos))
                        {
                            case HASH_KEY_NON_EXISTANT:
                                *in_key = 0;
                                break;
                            case HASH_KEY_IS_LONG:
                                *in_key = in_key_index;
                                break;
                            default:
                            {
                                zval *key;
                                MAKE_STD_ZVAL(key);
                                ZVAL_STRINGL(key, in_key_name, in_key_len, 1);
                                convert_to_long(key);
                                *in_key = Z_LVAL_P(key);
                                zval_ptr_dtor(&key);
                                break;
                            }
                        }
                        *in_values = *tmp;
                    }
                    else
                    {
                        *in_key = 0;
                        *in_values = *val;
                    }
                }
            }
            else
            {
                *in_key = 0;
                *in_values = *val;
            }
        }
        else if (strcmp(key, "filter") == 0 && filter != NULL)
        {
            /* filter */
            hs_zval_to_filter(
                filters, filter, *val, HS_PROTOCOL_FILTER TSRMLS_CC);
        }
        else if (strcmp(key, "while") == 0 && filter != NULL)
        {
            /* while */
            hs_zval_to_filter(
                filters, filter, *val, HS_PROTOCOL_WHILE TSRMLS_CC);
        }

        zend_hash_move_forward_ex(ht, &pos);
    }
}

static int hs_zval_to_operate_criteria(
    zval *query, zval *operate, zval **criteria, char *defaults TSRMLS_DC)
{
    if (query == NULL)
    {
        return -1;
    }

    if (Z_TYPE_P(query) == IS_ARRAY)
    {
        char *key;
        uint key_len;
        ulong index;
        HashTable *ht;
        zval **tmp;

        ht = HASH_OF(query);

        if (zend_hash_get_current_data_ex(ht, (void **)&tmp, NULL) != SUCCESS)
        {
            return -1;
        }

        if (zend_hash_get_current_key_ex(
                ht, &key, &key_len, &index, 0, NULL) == HASH_KEY_IS_STRING)
        {
            ZVAL_STRINGL(operate, key, key_len - 1, 1);
            *criteria = *tmp;
        }
        else
        {
            ZVAL_STRINGL(operate, defaults, strlen(defaults), 1);
            *criteria = query;
        }
    }
    else
    {
        ZVAL_STRINGL(operate, defaults, strlen(defaults), 1);
        *criteria = query;
    }

    return 0;
}

static int hs_request_find_execute(
    php_hs_t *hs, long id,
    zval *operate, zval *criteria,
    zval *update, zval *values, long field,
    long limit, long offset,
    zval *filters, long in_key, zval *in_values TSRMLS_DC)
{
    int ret = 0;
    dena::string_ref operate_str, update_str;
    std::vector<dena::string_ref> criteria_vec, in_vec, values_vec;
    std::vector<dena::hstcpcli_filter> filter_vec;

    convert_to_string(operate);
    operate_str = dena::string_ref(Z_STRVAL_P(operate), Z_STRLEN_P(operate));

    if (strcmp(Z_STRVAL_P(operate), HS_PROTOCOL_INSERT) == 0)
    {
        ret = 1;
    }

    if (Z_TYPE_P(criteria) == IS_ARRAY)
    {
        hs_array_to_vector(criteria, criteria_vec TSRMLS_CC);
    }
    else if (Z_TYPE_P(criteria) == IS_NULL)
    {
        criteria_vec.push_back(dena::string_ref());
    }
    else
    {
        convert_to_string(criteria);
        criteria_vec.push_back(
            dena::string_ref(Z_STRVAL_P(criteria), Z_STRLEN_P(criteria)));
    }

    /* update */
    if (update != NULL && Z_TYPE_P(update) != IS_NULL)
    {
        long len;

        convert_to_string(update);
        update_str = dena::string_ref(Z_STRVAL_P(update), Z_STRLEN_P(update));

        len = Z_STRLEN_P(update);
        if (len == 1 || len != 2)
        {
            ret = 1;
        }

        if (values != NULL)
        {
            if (Z_TYPE_P(values) == IS_ARRAY)
            {
                if (field <= 0 ||
                    zend_hash_num_elements(HASH_OF(values)) >= field)
                {
                    hs_array_to_vector(values, values_vec TSRMLS_CC);
                }
            }
            else
            {
                if (field <= 0 || field == 1)
                {
                    if (Z_TYPE_P(values) == IS_NULL)
                    {
                        values_vec.push_back(dena::string_ref());
                    }
                    else
                    {
                        convert_to_string(values);
                        values_vec.push_back(
                            dena::string_ref(
                                Z_STRVAL_P(values), Z_STRLEN_P(values)));
                    }
                }
            }
        }
    }

    /* in */
    if (in_key >= 0 && in_values != NULL)
    {
        if (Z_TYPE_P(in_values) == IS_ARRAY)
        {
            hs_array_to_vector(in_values, in_vec TSRMLS_CC);
        }
        else
        {
            convert_to_string(in_values);
            in_vec.push_back(
                dena::string_ref(Z_STRVAL_P(in_values), Z_STRLEN_P(in_values)));
        }
    }

    /* filter */
    if (filters != NULL && Z_TYPE_P(filters) == IS_ARRAY)
    {
        hs_array_to_vector_filter(filter_vec, HASH_OF(filters) TSRMLS_CC);
    }

    hs->cli->request_buf_exec_generic(
        id, operate_str,
        &criteria_vec[0], criteria_vec.size(),
        limit, offset,
        update_str, &values_vec[0], values_vec.size(),
        &filter_vec[0], filter_vec.size(),
        in_key, &in_vec[0], in_vec.size());

    return ret;
}

static void hs_response_value(
    php_hs_t *hs, zval *return_value, size_t count, int modify TSRMLS_DC)
{
    const dena::string_ref *row = 0;

    if (modify)
    {
        if ((row = hs->cli->get_next_row()) != 0)
        {
            const dena::string_ref& v = row[0];

            if (v.begin() != 0)
            {
                ZVAL_STRINGL(return_value, (char *)v.begin(), v.size(), 1);
                convert_to_long(return_value);
            }
            else
            {
                ZVAL_LONG(return_value, 1);
            }
        }
        else
        {
            ZVAL_LONG(return_value, 1);
        }
    }
    else
    {
        array_init(return_value);

        while ((row = hs->cli->get_next_row()) != 0)
        {
            size_t i;
            zval *value;

            ALLOC_INIT_ZVAL(value);

            array_init_size(value, count);

            for (i = 0; i < count; ++i)
            {
                const dena::string_ref& v = row[i];
                if (v.begin() != 0)
                {
                    add_next_index_stringl(
                        value, (char *)v.begin(), v.size(), 1);
                }
                else
                {
                    add_next_index_null(value);
                }
            }

            add_next_index_zval(return_value, value);
        }
    }
}

static int hs_is_options_safe(HashTable *options TSRMLS_DC)
{
    zval **tmp;

    if (zend_hash_find(
            options, "safe", sizeof("safe"), (void**)&tmp) == SUCCESS)
    {
        if (Z_TYPE_PP(tmp) == IS_STRING ||
            ((Z_TYPE_PP(tmp) == IS_LONG || Z_TYPE_PP(tmp) == IS_BOOL) &&
             Z_LVAL_PP(tmp) >= 1))
        {
            return 1;
        }
    }

    return 0;
}

/* HandlerSocket Class */
static void hs_free(php_hs_t *hs TSRMLS_DC)
{
    if (hs)
    {
        if (hs->cli)
        {
            hs->cli->close();
            delete hs->cli;
        }

        if (hs->error)
        {
            zval_ptr_dtor(&hs->error);
        }

        zend_object_std_dtor(&hs->object TSRMLS_CC);

        efree(hs);
    }
}

static zend_object_value hs_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    zval *tmp;
    php_hs_t *hs;

    hs = (php_hs_t *)emalloc(sizeof(php_hs_t));

    zend_object_std_init(&hs->object, ce TSRMLS_CC);
    zend_hash_copy(
        hs->object.properties, &ce->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(
        hs, (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)hs_free,
        NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    hs->cli = NULL;
    hs->error = NULL;
    return retval;
}

static ZEND_METHOD(HandlerSocket, __construct)
{
    char *host = NULL, *port = NULL;
    int host_len, port_len, server_len;
    zval *options = NULL;

    php_hs_t *hs;

    dena::config conf;
    dena::socket_args args;
    dena::hstcpcli_ptr cli;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "ss|z",
            &host, &host_len, &port, &port_len, &options) == FAILURE)
    {
        return;
    }

    if ((strlen(host) == 0 || strlen(port) == 0) &&  options == NULL)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] no server name or port given");
        RETURN_FALSE;
    }

    if (strlen(host) > 0)
    {
        conf["host"] = std::string(host);
    }

    if (strlen(port) > 0)
    {
        conf["port"] = std::string(port);
    }

    if (options && Z_TYPE_P(options) == IS_ARRAY)
    {
        hs_array_to_conf(HASH_OF(options), conf TSRMLS_CC);
    }

    args.set(conf);

    cli = dena::hstcpcli_i::create(args);
    hs->cli = cli.get();
    cli.release();

    if (hs->cli->get_error_code() < 0)
    {
        hs->cli->close();
        delete hs->cli;
        hs->cli = NULL;

        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to connect %s:%s", host, port);
        RETURN_FALSE;
    }
}

static ZEND_METHOD(HandlerSocket, auth)
{
    long key_len, type_len;
    char *key, *type;
    size_t num_flds;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
            &key, &key_len, &type, &type_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (key_len <= 0)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    hs->cli->request_buf_auth(key, "1"); /* type: 1 */
    if (hs->cli->get_error_code() < 0)
    {
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        RETURN_FALSE;
    }

    if (hs->cli->request_send() != 0 ||
        hs->cli->response_recv(num_flds) < 0)
    {
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        ZVAL_BOOL(return_value, 0);
    }
    else
    {
        if (hs->cli->get_error_code() != 0)
        {
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(),
                hs->cli->get_error().size(), 1);
            ZVAL_BOOL(return_value, 0);
        }
        else
        {
            ZVAL_BOOL(return_value, 1);
        }
    }

    hs->cli->response_buf_remove();
}

static ZEND_METHOD(HandlerSocket, openIndex)
{
    long id;
    char *db, *table, *index, *filters_cstr = NULL;
    int db_len, table_len, index_len;
    zval *field = NULL, *filters = NULL;

    std::string request_field, request_filter;
    size_t num_flds;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lsssz|z",
            &id, &db, &db_len, &table, &table_len,
            &index, &index_len, &field, &filters) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    /* field */
    if (Z_TYPE_P(field) == IS_ARRAY)
    {
        hs_array_to_string(request_field, HASH_OF(field) TSRMLS_CC);
    }
    else if (Z_TYPE_P(field) == IS_STRING)
    {
        request_field.append(Z_STRVAL_P(field), Z_STRLEN_P(field));
    }
    else
    {
        convert_to_string(field);
        request_field.append(Z_STRVAL_P(field), Z_STRLEN_P(field));
    }

    /* filter */
    if (filters)
    {
        if (Z_TYPE_P(filters) == IS_ARRAY)
        {
            hs_array_to_string(request_filter, HASH_OF(filters) TSRMLS_CC);
        }
        else
        {
            convert_to_string(filters);
            request_filter.append(Z_STRVAL_P(filters), Z_STRLEN_P(filters));
        }

        filters_cstr = (char *)request_filter.c_str();
    }

    hs->cli->request_buf_open_index(
        id, db, table, index, request_field.c_str(), filters_cstr);

    if (hs->cli->get_error_code() < 0)
    {
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        RETURN_FALSE;
    }

    if (hs->cli->request_send() != 0)
    {
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        RETURN_FALSE;
    }
    else
    {
        if (hs->cli->response_recv(num_flds) != 0)
        {
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
            ZVAL_BOOL(return_value, 0);
        }
        else
        {
            if (hs->cli->get_error_code() != 0)
            {
                ZVAL_STRINGL(
                    hs->error,
                    hs->cli->get_error().c_str(),
                    hs->cli->get_error().size(), 1);
                ZVAL_BOOL(return_value, 0);
            }
            else
            {
                ZVAL_BOOL(return_value, 1);
            }
        }

        hs->cli->response_buf_remove();
    }
}

static ZEND_METHOD(HandlerSocket, executeSingle)
{
    long id, operate_len, update_len = 0, in_key = -1;
    long limit = 1, offset = 0;
    char *operate, *update = NULL;
    zval *criteria, *values = NULL, *filters = NULL, *in_values = NULL;
    zval *find_operate = NULL, *find_update = NULL;
    int modify, field = -1;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lsz|llszzlz",
            &id, &operate, &operate_len, &criteria,
            &limit, &offset, &update, &update_len, &values, &filters,
            &in_key, &in_values) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(find_operate);
    ZVAL_STRINGL(find_operate, operate, operate_len, 1);

    /* modify */
    if (update_len > 0)
    {
        MAKE_STD_ZVAL(find_update);
        ZVAL_STRINGL(find_update, update, update_len, 1);

        if (values != NULL && Z_TYPE_P(values) == IS_ARRAY)
        {
            field = zend_hash_num_elements(HASH_OF(values));
        }
        else
        {
            field = -1;
        }
    }

    /* find */
    modify = hs_request_find_execute(
        hs, id, find_operate, criteria,
        find_update, values, field,
        limit, offset, filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, modify TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&find_operate);
    if (find_update)
    {
        zval_ptr_dtor(&find_update);
    }
}

static ZEND_METHOD(HandlerSocket, executeMulti)
{
    zval *args = NULL;
    zval *rmodify, **val;
    HashPosition pos;
    int err = -1;
    long i, n;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &args) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(rmodify);
    array_init(rmodify);

    zend_hash_internal_pointer_reset_ex(HASH_OF(args), &pos);
    while (zend_hash_get_current_data_ex(
               HASH_OF(args), (void **)&val, &pos) == SUCCESS)
    {
        HashTable *ht;
        long id, in_key = -1, limit = 1, offset = 0;
        long field, modify;
        zval **operate, **criteria, **tmp;
        zval *update = NULL, *values = NULL;
        zval *filters = NULL, *in_values = NULL;

        if (Z_TYPE_PP(val) != IS_ARRAY)
        {
            err = -1;
            break;
        }

        ht = HASH_OF(*val);

        /* 0: id */
        if (zend_hash_index_find(ht, 0, (void **)&tmp) != SUCCESS)
        {
            err = -1;
            break;
        }
        convert_to_long(*tmp);
        id = Z_LVAL_PP(tmp);

        /* 1: operate */
        if (zend_hash_index_find(ht, 1, (void **)&operate) != SUCCESS)
        {
            err = -1;
            break;
        }
        convert_to_string(*operate);

        /* 2: criteria */
        if (zend_hash_index_find(ht, 2, (void **)&criteria) != SUCCESS)
        {
            err = -1;
            break;
        }

        n = zend_hash_num_elements(ht);

        for (i = 3; i < n; i++)
        {
            if (zend_hash_index_find(ht, i, (void **)&tmp) == SUCCESS)
            {
                switch (i)
                {
                    case 3:
                        /* 3: limit */
                        convert_to_long(*tmp);
                        limit = Z_LVAL_PP(tmp);
                        break;
                    case 4:
                        /* 4: offset */
                        convert_to_long(*tmp);
                        offset = Z_LVAL_PP(tmp);
                        break;
                    case 5:
                        /* 5: update */
                        update = *tmp;
                        break;
                    case 6:
                        /* 6: values */
                        values = *tmp;
                        break;
                    case 7:
                        /* 7: filters */
                        filters = *tmp;
                        break;
                    case 8:
                        /* 8: in_key */
                        convert_to_long(*tmp);
                        in_key = Z_LVAL_PP(tmp);
                        break;
                    case 9:
                        /* 9: in_values */
                        in_values = *tmp;
                        break;
                    default:
                        break;
                }
            }
        }

        /* modify */
        if (update != NULL && Z_TYPE_P(update) != IS_NULL)
        {
            if (values != NULL && Z_TYPE_P(values) == IS_ARRAY)
            {
                field = zend_hash_num_elements(HASH_OF(values));
            }
            else
            {
                field = -1;
            }
        }

        /* find */
        modify = hs_request_find_execute(
            hs, id, *operate, *criteria, update, values, field,
            limit, offset, filters, in_key, in_values TSRMLS_CC);

        add_next_index_long(rmodify, modify);

        err = 0;

        zend_hash_move_forward_ex(HASH_OF(args), &pos);
    }

    /* request: send */
    if (err < 0)
    {
        zval_ptr_dtor(&rmodify);
        RETURN_FALSE;
    }

    if (hs->cli->request_send() != 0)
    {
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        ZVAL_BOOL(return_value, 0);
    }
    else
    {
        n = zend_hash_num_elements(HASH_OF(rmodify));

        array_init(return_value);
        array_init(hs->error);

        for (i = 0; i < n; i++)
        {
            zval **tmp;
            size_t num_flds, modify = 0;

            if (zend_hash_index_find(
                    HASH_OF(rmodify), i, (void **)&tmp) == SUCCESS)
            {
                modify = Z_LVAL_PP(tmp);
            }

            /* response: recv */
            if (hs->cli->response_recv(num_flds) != 0 ||
                hs->cli->get_error_code() != 0)
            {
                add_next_index_bool(return_value, 0);
                add_next_index_stringl(
                    hs->error,
                    hs->cli->get_error().c_str(),
                    hs->cli->get_error().size(), 1);
            }
            else
            {
                zval *item;
                MAKE_STD_ZVAL(item);

                hs_response_value(hs, item, num_flds, modify TSRMLS_CC);

                add_next_index_zval(return_value, item);
                add_next_index_null(hs->error);
            }

            hs->cli->response_buf_remove();
        }
    }

    zval_ptr_dtor(&rmodify);
}

static ZEND_METHOD(HandlerSocket, executeUpdate)
{
    long id, operate_len, in_key = -1;
    long limit = 1, offset = 0;
    char *operate;
    zval *criteria, *values = NULL, *filters = NULL, *in_values = NULL;
    zval *find_update, *find_operate = NULL;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lszz|llzlz",
            &id, &operate, &operate_len, &criteria,
            &values, &limit, &offset, &filters, &in_key, &in_values) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(find_operate);
    ZVAL_STRINGL(find_operate, operate, operate_len, 1);

    /* modify */
    MAKE_STD_ZVAL(find_update);
    ZVAL_STRINGL(find_update, HS_MODIFY_UPDATE, 1, 1);

    /* find */
    hs_request_find_execute(
        hs, id, find_operate, criteria,
        find_update, values, -1,
        limit, offset, filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 1 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&find_operate);
    zval_ptr_dtor(&find_update);
}

static ZEND_METHOD(HandlerSocket, executeDelete)
{
    long id, operate_len, in_key = -1;
    long limit = 1, offset = 0;
    char *operate;
    zval *criteria, *values = NULL, *filters = NULL, *in_values = NULL;
    zval *find_update, *find_operate = NULL;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lsz|llzlz",
            &id, &operate, &operate_len, &criteria,
            &limit, &offset, &filters, &in_key, &in_values) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(find_operate);
    ZVAL_STRINGL(find_operate, operate, operate_len, 1);

    /* modify */
    MAKE_STD_ZVAL(find_update);
    ZVAL_STRINGL(find_update, HS_MODIFY_REMOVE, 1, 1);

    MAKE_STD_ZVAL(values);
    ZVAL_NULL(values);

    /* find */
    hs_request_find_execute(
        hs, id, find_operate, criteria,
        find_update, values, -1,
        limit, offset, filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 1 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&find_operate);
    zval_ptr_dtor(&find_update);
    zval_ptr_dtor(&values);
}

static ZEND_METHOD(HandlerSocket, executeInsert)
{
    long id, n;
    zval *operate, *field;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lz", &id, &field) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(field) != IS_ARRAY ||
        zend_hash_num_elements(HASH_OF(field)) <= 0)
    {
        RETURN_FALSE;
    }

    if (!hs->cli)
    {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(operate);
    ZVAL_STRINGL(operate, HS_PROTOCOL_INSERT, 1, 1);

    /* find */
    hs_request_find_execute(
        hs, id, operate, field, NULL, NULL, -1, 0, 0, NULL, -1, NULL TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hs->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hs->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 1 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&operate);
}

static ZEND_METHOD(HandlerSocket, getError)
{
    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocketIndex);

    if (hs->error == NULL)
    {
        RETURN_NULL();
    }
    else
    {
        RETVAL_ZVAL(hs->error, 1, 0);
    }
}

static ZEND_METHOD(HandlerSocket, createIndex)
{
    long id;
    char *db, *table, *index;
    int db_len, table_len, index_len;
    zval *fields = NULL, *options = NULL;
    zval temp;
    zval *id_z, *db_z, *table_z, *index_z, *opt_z = NULL;

    php_hs_t *hs;

    hs = (php_hs_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);
    HS_ERROR_RESET(hs->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "lsssz|z",
            &id, &db, &db_len, &table, &table_len,
            &index, &index_len, &fields, &options) == FAILURE)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] expects parameters");
        RETURN_FALSE;
    }

    if (options != NULL)
    {
        if (Z_TYPE_P(options) == IS_STRING)
        {
            MAKE_STD_ZVAL(opt_z);
            array_init(opt_z);
            add_assoc_zval(opt_z, "filter", options);
        }
    }

    MAKE_STD_ZVAL(id_z);
    MAKE_STD_ZVAL(db_z);
    MAKE_STD_ZVAL(table_z);
    MAKE_STD_ZVAL(index_z);

    ZVAL_LONG(id_z, id);
    ZVAL_STRINGL(db_z, db, db_len, 1);
    ZVAL_STRINGL(table_z, table, table_len, 1);
    ZVAL_STRINGL(index_z, index, index_len, 1);

    object_init_ex(return_value, hs_index_ce);

    if (options == NULL)
    {
        HS_METHOD6(
            HandlerSocketIndex, __construct, &temp, return_value,
            getThis(), id_z, db_z, table_z, index_z, fields);
    }
    else if (opt_z != NULL)
    {
        HS_METHOD7(
            HandlerSocketIndex, __construct, &temp, return_value,
            getThis(), id_z, db_z, table_z, index_z, fields, opt_z);
        zval_ptr_dtor(&opt_z);
    }
    else
    {
        HS_METHOD7(
            HandlerSocketIndex, __construct, &temp, return_value,
            getThis(), id_z, db_z, table_z, index_z, fields, options);
    }

    zval_ptr_dtor(&id_z);
    zval_ptr_dtor(&db_z);
    zval_ptr_dtor(&table_z);
    zval_ptr_dtor(&index_z);
}

/* HandlerSocket Index Class */
static void hs_index_free(php_hs_index_t *hsi TSRMLS_DC)
{
    if (hsi)
    {
        if (hsi->link)
        {
            zval_ptr_dtor(&hsi->link);
        }

        if (hsi->filter)
        {
            zval_ptr_dtor(&hsi->filter);
        }

        if (hsi->error)
        {
            zval_ptr_dtor(&hsi->error);
        }

        zend_object_std_dtor(&hsi->object TSRMLS_CC);

        efree(hsi);
    }
}

static zend_object_value hs_index_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    zval *tmp;
    php_hs_index_t *hsi;

    hsi = (php_hs_index_t *)emalloc(sizeof(php_hs_index_t));

    zend_object_std_init(&hsi->object, ce TSRMLS_CC);
    zend_hash_copy(
        hsi->object.properties, &ce->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(
        hsi, (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)hs_index_free,
        NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    hsi->id = 0;
    hsi->link = NULL;
    hsi->filter = NULL;
    hsi->error = NULL;

    return retval;
}

static ZEND_METHOD(HandlerSocketIndex, __construct)
{
    zval *link;
    long id;
    char *db, *table, *index, *filters_cstr = NULL;
    int db_len, table_len, index_len;
    zval *fields, *opts = NULL;
    size_t num_flds;

    std::string request_field, request_filter;

    php_hs_t *hs;
    php_hs_index_t *hsi;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "Olsssz|z",
            &link, hs_ce, &id, &db, &db_len, &table, &table_len,
            &index, &index_len, &fields, &opts) == FAILURE)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] expects parameters");
        return;
    }

    hs = (php_hs_t *)zend_object_store_get_object(link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", id);
        RETURN_FALSE;
    }

    hsi->link = link;
    zval_add_ref(&hsi->link);

    if (Z_TYPE_P(fields) == IS_ARRAY)
    {
        hs_array_to_string(request_field, HASH_OF(fields) TSRMLS_CC);
    }
    else if (Z_TYPE_P(fields) == IS_STRING)
    {
        request_field.append(Z_STRVAL_P(fields), Z_STRLEN_P(fields));
    }
    else
    {
        convert_to_string(fields);
        request_field.append(Z_STRVAL_P(fields), Z_STRLEN_P(fields));
    }

    if (opts)
    {
        zval **tmp;

        convert_to_array(opts);
        if (zend_hash_find(
                HASH_OF(opts), "filter", sizeof("filter"),
                (void **)&tmp) == SUCCESS)
        {
            MAKE_STD_ZVAL(hsi->filter);

            if (Z_TYPE_PP(tmp) == IS_ARRAY)
            {
                long n, i;

                hs_array_to_string(request_filter, HASH_OF(*tmp) TSRMLS_CC);

                array_init(hsi->filter);

                n = zend_hash_num_elements(HASH_OF(*tmp));
                for (i = 0; i < n; i++)
                {
                    zval **val;
                    if (zend_hash_index_find(
                            HASH_OF(*tmp), i, (void **)&val) == SUCCESS)
                    {
                        convert_to_string(*val);
                        add_next_index_stringl(
                            hsi->filter, Z_STRVAL_PP(val), Z_STRLEN_PP(val), 1);
                    }
                }
            }
            else if (Z_TYPE_PP(tmp) == IS_STRING)
            {
                zval delim;

                convert_to_string(*tmp);
                ZVAL_STRINGL(&delim, ",", strlen(","), 0);

                request_filter.append(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));

                array_init(hsi->filter);
                php_explode(&delim, *tmp, hsi->filter, LONG_MAX);
            }
            else
            {
                ZVAL_NULL(hsi->filter);
            }

            filters_cstr = (char *)request_filter.c_str();
        }
    }

    hs->cli->request_buf_open_index(
        id, db, table, index, request_field.c_str(), filters_cstr);

    if (hs->cli->get_error_code() < 0 ||
        hs->cli->request_send() != 0)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld: %s",
            id, hs->cli->get_error().c_str());
        RETURN_FALSE;
    }

    if (hs->cli->response_recv(num_flds) != 0 ||
        hs->cli->get_error_code() != 0)
    {
        hs->cli->response_buf_remove();

        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld: %s",
            id, hs->cli->get_error().c_str());
        RETURN_FALSE;
    }

    hs->cli->response_buf_remove();

    /* property */
    zend_update_property_stringl(
        hs_index_ce, getThis(), "_db", strlen("_db"),
        db, db_len TSRMLS_CC);
    zend_update_property_stringl(
        hs_index_ce, getThis(), "_table", strlen("_table"),
        table, table_len TSRMLS_CC);
    zend_update_property_stringl(
        hs_index_ce, getThis(), "_name", strlen("_name"),
        index, index_len TSRMLS_CC);
    zend_update_property(
        hs_index_ce, getThis(), "_field", strlen("_field"), fields TSRMLS_CC);

    /* id */
    hsi->id = id;
}

static ZEND_METHOD(HandlerSocketIndex, getId)
{
    php_hs_index_t *hsi;
    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);

    RETVAL_LONG(hsi->id);
}
static ZEND_METHOD(HandlerSocketIndex, getDatabase)
{
    zval *prop;
    HS_INDEX_PROPERTY(_db, prop);
    RETVAL_ZVAL(prop, 1, 0);
}

static ZEND_METHOD(HandlerSocketIndex, getTable)
{
    zval *prop;
    HS_INDEX_PROPERTY(_table, prop);
    RETVAL_ZVAL(prop, 1, 0);
}

static ZEND_METHOD(HandlerSocketIndex, getName)
{
    zval *prop;
    HS_INDEX_PROPERTY(_name, prop);
    RETVAL_ZVAL(prop, 1, 0);
}

static ZEND_METHOD(HandlerSocketIndex, getField)
{
    zval *prop;

    HS_INDEX_PROPERTY(_field, prop);

    if (Z_TYPE_P(prop) == IS_STRING)
    {
        zval delim;

        array_init(return_value);
        ZVAL_STRINGL(&delim, ",", strlen(","), 0);

        php_explode(&delim, prop, return_value, LONG_MAX);
    }
    else
    {
        RETVAL_ZVAL(prop, 1, 0);
    }
}

static ZEND_METHOD(HandlerSocketIndex, getFilter)
{
    php_hs_index_t *hsi;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);

    if (!hsi->filter)
    {
        RETURN_NULL();
    }

    RETVAL_ZVAL(hsi->filter, 1, 0);
}

static ZEND_METHOD(HandlerSocketIndex, getOperator)
{
    zval *find, *modify;

    MAKE_STD_ZVAL(find);
    array_init(find);

    add_next_index_stringl(
        find, HS_FIND_EQUAL, strlen(HS_FIND_EQUAL), 1);
    add_next_index_stringl(
        find, HS_FIND_LESS, strlen(HS_FIND_LESS), 1);
    add_next_index_stringl(
        find, HS_FIND_LESS_EQUAL, strlen(HS_FIND_LESS_EQUAL), 1);
    add_next_index_stringl(
        find, HS_FIND_GREATER, strlen(HS_FIND_GREATER), 1);
    add_next_index_stringl(
        find, HS_FIND_GREATER_EQUAL, strlen(HS_FIND_GREATER_EQUAL), 1);

    MAKE_STD_ZVAL(modify);
    array_init(modify);

    add_next_index_stringl(
        modify, HS_MODIFY_UPDATE, strlen(HS_MODIFY_UPDATE), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_INCREMENT, strlen(HS_MODIFY_INCREMENT), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_DECREMENT, strlen(HS_MODIFY_DECREMENT), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_REMOVE, strlen(HS_MODIFY_REMOVE), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_UPDATE_PREV, strlen(HS_MODIFY_UPDATE_PREV), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_INCREMENT_PREV, strlen(HS_MODIFY_INCREMENT_PREV), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_DECREMENT_PREV, strlen(HS_MODIFY_DECREMENT_PREV), 1);
    add_next_index_stringl(
        modify, HS_MODIFY_REMOVE_PREV, strlen(HS_MODIFY_REMOVE_PREV), 1);

    array_init(return_value);
    add_next_index_zval(return_value, find);
    add_next_index_zval(return_value, modify);
}

static ZEND_METHOD(HandlerSocketIndex, getError)
{
    php_hs_index_t *hsi;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);

    if (hsi->error == NULL)
    {
        RETURN_NULL();
    }
    else
    {
        RETVAL_ZVAL(hsi->error, 1, 0);
    }
}

static ZEND_METHOD(HandlerSocketIndex, find)
{
    zval *query, *operate, *criteria;
    long limit = 1, offset = 0;
    zval *options = NULL;

    zval *filters = NULL, *in_values = NULL;
    long in_key = -1;

    int safe = -1;

    php_hs_index_t *hsi;
    php_hs_t *hs;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "z|llz",
            &query, &limit, &offset, &options) == FAILURE)
    {
        RETURN_FALSE;
    }

    hs = (php_hs_t *)zend_object_store_get_object(hsi->link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", hsi->id);
        RETURN_FALSE;
    }

    /* operete : criteria */
    MAKE_STD_ZVAL(operate);
    if (hs_zval_to_operate_criteria(
            query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
    {
        zval_ptr_dtor(&operate);
        RETURN_FALSE;
    }

    if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
    {
        /* options: safe */
        safe = hs_is_options_safe(HASH_OF(options) TSRMLS_CC);

        /* options: filters, in_key, in_values */
        hs_array_to_in_filter(
            HASH_OF(options), hsi->filter,
            &filters, &in_key, &in_values TSRMLS_CC);
    }

    /* find */
    hs_request_find_execute(
        hs, hsi->id, operate, criteria,
        NULL, NULL, -1, limit, offset, filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_STRINGL(
            hsi->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        ZVAL_BOOL(return_value, 0);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hsi->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 0 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&operate);
    if (filters)
    {
        zval_ptr_dtor(&filters);
    }

    /* exception */
    if (safe > 0 &&
        Z_TYPE_P(return_value) == IS_BOOL && Z_LVAL_P(return_value) == 0)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] response error: %s",
            hsi->error == NULL ? "Unknown error" : Z_STRVAL_P(hsi->error));
    }
}

static ZEND_METHOD(HandlerSocketIndex, insert)
{
    zval ***args;
    long i, argc = ZEND_NUM_ARGS();
    zval *operate, *fields;

    php_hs_index_t *hsi;
    php_hs_t *hs;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (argc < 1)
    {
        zend_wrong_param_count(TSRMLS_C);
        RETURN_FALSE;
    }

    args = (zval ***)safe_emalloc(argc, sizeof(zval **), 0);
    if (zend_get_parameters_array_ex(argc, args) == FAILURE)
    {
        efree(args);
        zend_wrong_param_count(TSRMLS_C);
        RETURN_FALSE;
    }

    hs = (php_hs_t *)zend_object_store_get_object(hsi->link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        efree(args);
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", hsi->id);
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(operate);
    ZVAL_STRINGL(operate, HS_PROTOCOL_INSERT, 1, 1);

    if (Z_TYPE_PP(args[0]) == IS_ARRAY)
    {
        fields = *args[0];
    }
    else
    {
        MAKE_STD_ZVAL(fields);
        array_init(fields);

        for (i = 0; i < argc; i++)
        {
            switch (Z_TYPE_P(*args[i]))
            {
                case IS_NULL:
                    add_next_index_null(fields);
                    break;
                case IS_LONG:
                    add_next_index_long(fields, Z_LVAL_P(*args[i]));
                    break;
                case IS_DOUBLE:
                    add_next_index_double(fields, Z_DVAL_P(*args[i]));
                    break;
                default:
                    convert_to_string(*args[i]);
                    add_next_index_stringl(
                        fields, Z_STRVAL_P(*args[i]), Z_STRLEN_P(*args[i]), 1);
                    break;
            }
        }
    }

    /* find */
    hs_request_find_execute(
        hs, hsi->id, operate, fields,
        NULL, NULL, -1, 0, 0, NULL, -1, NULL TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hsi->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hsi->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 1 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&operate);
    zval_ptr_dtor(&fields);
    efree(args);
}

static ZEND_METHOD(HandlerSocketIndex, update)
{
    zval *query, *update, *operate, *criteria, *modify_operate, *modify_criteria;
    zval *options = NULL;
    long limit = 1, offset = 0;

    zval *filters = NULL, *in_values = NULL;
    long in_key = -1;

    int safe = -1;
    int modify = 1;

    php_hs_index_t *hsi;
    php_hs_t *hs;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "zz|llz",
            &query, &update, &limit, &offset, &options) == FAILURE)
    {
        RETURN_FALSE;
    }

    hs = (php_hs_t *)zend_object_store_get_object(hsi->link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", hsi->id);
        RETURN_FALSE;
    }

    /* operete : criteria */
    MAKE_STD_ZVAL(operate);
    if (hs_zval_to_operate_criteria(
            query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
    {
        zval_ptr_dtor(&operate);
        RETURN_FALSE;
    }

    /* modify_operete : modify_criteria */
    MAKE_STD_ZVAL(modify_operate);
    if (hs_zval_to_operate_criteria(
            update, modify_operate, &modify_criteria,
            HS_MODIFY_UPDATE TSRMLS_CC) < 0)
    {
        zval_ptr_dtor(&operate);
        zval_ptr_dtor(&modify_operate);
        RETURN_FALSE;
    }

    if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
    {
        /* options: safe */
        safe = hs_is_options_safe(HASH_OF(options) TSRMLS_CC);

        /* options: filters, in_key, in_values */
        hs_array_to_in_filter(
            HASH_OF(options), hsi->filter,
            &filters, &in_key, &in_values TSRMLS_CC);
    }

    /* find */
    modify = hs_request_find_execute(
        hs, hsi->id, operate, criteria,
        modify_operate, modify_criteria,
        -1, limit, offset, filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hsi->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hsi->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, modify TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&operate);
    zval_ptr_dtor(&modify_operate);
    if (filters)
    {
        zval_ptr_dtor(&filters);
    }

    /* exception */
    if (safe > 0 &&
        Z_TYPE_P(return_value) == IS_BOOL && Z_LVAL_P(return_value) == 0)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] response error: %s",
            hsi->error == NULL ? "Unknown error" : Z_STRVAL_P(hsi->error));
    }
}

static ZEND_METHOD(HandlerSocketIndex, remove)
{
    zval *query, *operate, *criteria, *options = NULL;
    long limit = 1, offset = 0;

    zval *remove, *filters = NULL, *in_values = NULL;
    long in_key = -1;

    int safe = -1;

    php_hs_index_t *hsi;
    php_hs_t *hs;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC, "z|llz",
            &query, &limit, &offset, &options) == FAILURE)
    {
        RETURN_FALSE;
    }

    hs = (php_hs_t *)zend_object_store_get_object(hsi->link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", hsi->id);
        RETURN_FALSE;
    }

    /* operete : criteria */
    MAKE_STD_ZVAL(operate);
    if (hs_zval_to_operate_criteria(
            query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
    {
        zval_ptr_dtor(&operate);
        RETURN_FALSE;
    }

    if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
    {
        /* options: safe */
        safe = hs_is_options_safe(HASH_OF(options) TSRMLS_CC);

        /* options: filters, in_key, in_values */
        hs_array_to_in_filter(
            HASH_OF(options), hsi->filter,
            &filters, &in_key, &in_values TSRMLS_CC);
    }

    MAKE_STD_ZVAL(remove);
    ZVAL_STRINGL(remove, HS_MODIFY_REMOVE, 1, 1);

    /* find */
    hs_request_find_execute(
        hs, hsi->id, operate, criteria,
        remove, NULL, -1, limit, offset,
        filters, in_key, in_values TSRMLS_CC);

    /* request: send */
    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hsi->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        size_t num_flds;

        /* response: recv */
        if (hs->cli->response_recv(num_flds) != 0 ||
            hs->cli->get_error_code() != 0)
        {
            ZVAL_BOOL(return_value, 0);
            ZVAL_STRINGL(
                hsi->error,
                hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
        }
        else
        {
            hs_response_value(hs, return_value, num_flds, 1 TSRMLS_CC);
        }

        hs->cli->response_buf_remove();
    }

    zval_ptr_dtor(&operate);
    zval_ptr_dtor(&remove);
    if (filters)
    {
        zval_ptr_dtor(&filters);
    }

    /* exception */
    if (safe > 0 &&
        Z_TYPE_P(return_value) == IS_BOOL && Z_LVAL_P(return_value) == 0)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] response error: %s",
            hsi->error == NULL ? "Unknown error" : Z_STRVAL_P(hsi->error));
    }
}

static ZEND_METHOD(HandlerSocketIndex, multi)
{
    zval *args = NULL;
    zval *rmodify;
    HashPosition pos;
    zval **val;

    php_hs_index_t *hsi;
    php_hs_t *hs;

    int err = -1;
    long i, n;

    hsi = (php_hs_index_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
    HS_CHECK_OBJECT(hsi, HandlerSocketIndex);
    HS_ERROR_RESET(hsi->error);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &args) == FAILURE)
    {
        RETURN_FALSE;
    }

    hs = (php_hs_t *)zend_object_store_get_object(hsi->link TSRMLS_CC);
    HS_CHECK_OBJECT(hs, HandlerSocket);

    if (!hs->cli)
    {
        zend_throw_exception_ex(
            hs_exception_ce, 0 TSRMLS_CC,
            "[handlersocket] unable to open index: %ld", hsi->id);
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(rmodify);
    array_init(rmodify);

    zend_hash_internal_pointer_reset_ex(HASH_OF(args), &pos);
    while (zend_hash_get_current_data_ex(
               HASH_OF(args), (void **)&val, &pos) == SUCCESS)
    {
        zval **method;
        HashTable *ht;

        if (Z_TYPE_PP(val) != IS_ARRAY)
        {
            err = -1;
            break;
        }

        ht = HASH_OF(*val);

        /* 0: method */
        if (zend_hash_index_find(ht, 0, (void **)&method) != SUCCESS)
        {
            err = -1;
            break;
        }

        convert_to_string(*method);

        if (strcmp(Z_STRVAL_PP(method), "find") == 0)
        {
            /* method: find */
            zval **query, **tmp, *operate, *criteria;
            zval *options = NULL, *filters = NULL, *in_values = NULL;
            long limit = 1, offset = 0, in_key = -1;

            n = zend_hash_num_elements(ht);
            if (n <= 1)
            {
                err = -1;
                break;
            }

            /* 1: query */
            if (zend_hash_index_find(ht, 1, (void **)&query) != SUCCESS)
            {
                err = -1;
                break;
            }

            /* operete : criteria */
            MAKE_STD_ZVAL(operate);
            if (hs_zval_to_operate_criteria(
                    *query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
            {
                err = -1;
                zval_ptr_dtor(&operate);
                break;
            }

            if (*query == NULL)
            {
                err = -1;
                zval_ptr_dtor(&operate);
                break;
            }

            for (i = 2; i < n; i++)
            {
                if (zend_hash_index_find(ht, i, (void **)&tmp) == SUCCESS)
                {
                    switch (i)
                    {
                        case 2:
                            /* 2: limit */
                            convert_to_long(*tmp);
                            limit = Z_LVAL_PP(tmp);
                            break;
                        case 3:
                            /* 3: offset */
                            convert_to_long(*tmp);
                            offset = Z_LVAL_PP(tmp);
                            break;
                        case 4:
                            /* 4: options */
                            options = *tmp;
                            break;
                        default:
                            break;
                    }
                }
            }

            if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
            {
                /* options: filters, in_key, in_values */
                hs_array_to_in_filter(
                    HASH_OF(options), hsi->filter,
                    &filters, &in_key, &in_values TSRMLS_CC);
            }

            /* find */
            hs_request_find_execute(
                hs, hsi->id, operate, criteria,
                NULL, NULL, -1, limit, offset,
                filters, in_key, in_values TSRMLS_CC);

            add_next_index_long(rmodify, 0);
            err = 0;

            zval_ptr_dtor(&operate);
            if (filters)
            {
                zval_ptr_dtor(&filters);
            }
        }
        else if (strcmp(Z_STRVAL_PP(method), "insert") == 0)
        {
            /* method: insert */
            zval *operate, *fields;
            zval **tmp;

            n = zend_hash_num_elements(ht);

            if (n <= 1)
            {
                err = -1;
                break;
            }

            if (zend_hash_index_find(ht, 1, (void **)&tmp) != SUCCESS)
            {
                err = -1;
                break;
            }

            MAKE_STD_ZVAL(fields);
            array_init(fields);

            if (Z_TYPE_PP(tmp) == IS_ARRAY)
            {
                n = zend_hash_num_elements(HASH_OF(*tmp));
                for (i = 0; i < n; i++)
                {
                    zval **val;
                    if (zend_hash_index_find(
                            HASH_OF(*tmp), i, (void **)&val) == SUCCESS)
                    {
                        if (Z_TYPE_PP(val) == IS_NULL)
                        {
                            add_next_index_null(fields);
                        }
                        else
                        {
                            convert_to_string(*val);
                            add_next_index_stringl(
                                fields, Z_STRVAL_PP(val), Z_STRLEN_PP(val), 1);
                        }
                    }
                }
            }
            else
            {
                i = 1;
                do
                {
                    if (Z_TYPE_PP(tmp) == IS_NULL)
                    {
                        add_next_index_null(fields);
                    }
                    else
                    {
                        convert_to_string(*tmp);
                        add_next_index_stringl(
                            fields, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
                    }

                    i++;

                    if (zend_hash_index_find(ht, i, (void **)&tmp) != SUCCESS)
                    {
                        break;
                    }
                }
                while (i < n);
            }

            MAKE_STD_ZVAL(operate);
            ZVAL_STRINGL(operate, HS_PROTOCOL_INSERT, 1, 1);

            /* find */
            hs_request_find_execute(
                hs, hsi->id, operate, fields,
                NULL, NULL, -1, 0, 0, NULL, -1, NULL TSRMLS_CC);

            add_next_index_long(rmodify, 1);
            err = 0;

            zval_ptr_dtor(&operate);
            zval_ptr_dtor(&fields);
        }
        else if (strcmp(Z_STRVAL_PP(method), "remove") == 0)
        {
            /* method: remove */
            zval **query, **tmp, *operate, *criteria, *remove;
            zval *options = NULL, *filters = NULL, *in_values = NULL;
            long limit = 1, offset = 0, in_key = -1;

            n = zend_hash_num_elements(ht);
            if (n <= 1)
            {
                err = -1;
                break;
            }

            /* 1: query */
            if (zend_hash_index_find(ht, 1, (void **)&query) != SUCCESS)
            {
                if (operate)
                {
                    zval_ptr_dtor(&operate);
                }
                err = -1;
                break;
            }

            /* operete : criteria */
            MAKE_STD_ZVAL(operate);
            if (hs_zval_to_operate_criteria(
                    *query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
            {
                zval_ptr_dtor(&operate);
                err = -1;
                break;
            }

            for (i = 2; i < n; i++)
            {
                if (zend_hash_index_find(ht, i, (void **)&tmp) == SUCCESS)
                {
                    switch (i)
                    {
                        case 2:
                            /* 2: limit */
                            convert_to_long(*tmp);
                            limit = Z_LVAL_PP(tmp);
                            break;
                        case 3:
                            /* 3: offset */
                            convert_to_long(*tmp);
                            offset = Z_LVAL_PP(tmp);
                            break;
                        case 4:
                            /* 4: options */
                            options = *tmp;
                            break;
                        default:
                            break;
                    }
                }
            }

            if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
            {
                /* options: filters, in_key, in_values */
                hs_array_to_in_filter(
                    HASH_OF(options), hsi->filter,
                    &filters, &in_key, &in_values TSRMLS_CC);
            }

            MAKE_STD_ZVAL(remove);
            ZVAL_STRINGL(remove, HS_MODIFY_REMOVE, 1, 1);

            /* find */
            hs_request_find_execute(
                hs, hsi->id, operate, criteria,
                remove, NULL, -1, limit, offset,
                filters, in_key, in_values TSRMLS_CC);

            add_next_index_long(rmodify, 1);
            err = 0;

            zval_ptr_dtor(&remove);
            zval_ptr_dtor(&operate);
            if (filters)
            {
                zval_ptr_dtor(&filters);
            }
        }
        else if (strcmp(Z_STRVAL_PP(method), "update") == 0)
        {
            /* method: update */
            zval **query, **update, **tmp;
            zval *operate, *criteria, *modify_operate, *modify_criteria;
            zval *options = NULL, *filters = NULL, *in_values = NULL;
            long limit = 1, offset = 0, in_key = -1;
            int modify;

            n = zend_hash_num_elements(ht);
            if (n <= 2)
            {
                err = -1;
                break;
            }

            /* 1: query */
            if (zend_hash_index_find(ht, 1, (void **)&query) != SUCCESS)
            {
                err = -1;
                break;
            }

            /* 2: update */
            if (zend_hash_index_find(ht, 2, (void **)&update) != SUCCESS)
            {
                err = -1;
                break;
            }

            /* operete : criteria */
            MAKE_STD_ZVAL(operate);
            if (hs_zval_to_operate_criteria(
                    *query, operate, &criteria, HS_FIND_EQUAL TSRMLS_CC) < 0)
            {
                zval_ptr_dtor(&operate);
                err = -1;
                break;
            }

            /* modify_operete : modify_criteria */
            MAKE_STD_ZVAL(modify_operate);
            if (hs_zval_to_operate_criteria(
                    *update, modify_operate, &modify_criteria,
                    HS_MODIFY_UPDATE TSRMLS_CC) < 0)
            {
                zval_ptr_dtor(&operate);
                zval_ptr_dtor(&modify_operate);
                err = -1;
                break;
            }

            for (i = 3; i < n; i++)
            {
                if (zend_hash_index_find(ht, i, (void **)&tmp) == SUCCESS)
                {
                    switch (i)
                    {
                        case 3:
                            /* 3: limit */
                            convert_to_long(*tmp);
                            limit = Z_LVAL_PP(tmp);
                            break;
                        case 4:
                            /* 4: offset */
                            convert_to_long(*tmp);
                            offset = Z_LVAL_PP(tmp);
                            break;
                        case 5:
                            /* 5: options */
                            options = *tmp;
                            break;
                        default:
                            break;
                    }
                }
            }

            if (options != NULL && Z_TYPE_P(options) == IS_ARRAY)
            {
                /* options: filters, in_key, in_values */
                hs_array_to_in_filter(
                    HASH_OF(options), hsi->filter,
                    &filters, &in_key, &in_values TSRMLS_CC);
            }

            /* find */
            modify = hs_request_find_execute(
                hs, hsi->id, operate, criteria,
                modify_operate, modify_criteria, -1, limit, offset,
                filters, in_key, in_values TSRMLS_CC);

            add_next_index_long(rmodify, modify);
            err = 0;

            zval_ptr_dtor(&operate);
            zval_ptr_dtor(&modify_operate);
            if (filters)
            {
                zval_ptr_dtor(&filters);
            }
        }
        else
        {
            err = -1;
            break;
        }

        zend_hash_move_forward_ex(HASH_OF(args), &pos);
    }

    /* request: send */
    if (err < 0)
    {
        zval_ptr_dtor(&rmodify);
        RETURN_FALSE;
    }

    if (hs->cli->request_send() != 0)
    {
        ZVAL_BOOL(return_value, 0);
        ZVAL_STRINGL(
            hsi->error,
            hs->cli->get_error().c_str(), hs->cli->get_error().size(), 1);
    }
    else
    {
        n = zend_hash_num_elements(HASH_OF(rmodify));

        array_init(return_value);
        array_init(hsi->error);

        for (i = 0; i < n; i++)
        {
            zval **tmp;
            size_t num_flds, modify = 0;

            if (zend_hash_index_find(
                    HASH_OF(rmodify), i, (void **)&tmp) == SUCCESS)
            {
                modify = Z_LVAL_PP(tmp);
            }

            /* response: recv */
            if (hs->cli->response_recv(num_flds) != 0 ||
                hs->cli->get_error_code() != 0)
            {
                add_next_index_bool(return_value, 0);
                add_next_index_stringl(
                    hsi->error,
                    hs->cli->get_error().c_str(),
                    hs->cli->get_error().size(), 1);
            }
            else
            {
                zval *response;
                MAKE_STD_ZVAL(response);

                hs_response_value(hs, response, num_flds, modify TSRMLS_CC);

                add_next_index_zval(return_value, response);
                add_next_index_null(hsi->error);
            }

            hs->cli->response_buf_remove();
        }
    }

    zval_ptr_dtor(&rmodify);
}
