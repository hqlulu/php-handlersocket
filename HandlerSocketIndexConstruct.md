# HandlersocketIndex::construct #

Handlersocket::construct - Creates a new index objects

# Description #

```
public HandlerSocketIndex::__construct( HandlerSocket $hs, long $id, string $db, string $table, string $index, string|array $fields [, array $options = array() ] )
```

# Parameters #

## hs ##
| _object_ (HandlerSocket) | |
|:-------------------------|:|

## id ##
| _long_ | id |
|:-------|:---|

## db ##
| _string_ | database name |
|:---------|:--------------|

## table ##
| _string_ | table name |
|:---------|:-----------|

## index ##
| _string_ | index name |
|:---------|:-----------|

## fields ##
| _string_ or _array_ | field lists |
|:--------------------|:------------|

## options ##
| _array_ | |
|:--------|:|

# Return Values #

Returns the HandlerSocketIndex object.

# Errors/Exceptions #

Throws HandlerSocetException if the create is invalid.