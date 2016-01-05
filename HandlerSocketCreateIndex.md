# Handlersocket::createIndex #

Handlersocket::createIndex - Creates a new index objects

# Description #

```
public HandlerSocketIndex HandlerSocket::createIndex( long $id, string $db, string $table, string $index, string|array $fields [, array $options = array() ] )
```

# Parameters #

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
| _array_ | options |
|:--------|:--------|

  * filter
> filter lists.

# Return Values #

Returns the HandlerSocketIndex object.

# Errors/Exceptions #

Throws HandlerSocetException if the create is invalid.

# Examples #

```
<?php

try {
  $hs = new Handlersocker('localhost', 9999);

  $index = $hs->createIndex(1, 'db', 'table', 'PRIMARY', 'k,v');

} catch (HandlerSocketException $e) {
  die($e->getMessage());
}

?>
```

## filter setting ##

```
<?php

$index = $hs->createIndex(1, 'db', 'table', 'PRIMARY', array('k', 'v'), array('filter' => array('f1', 'f2')));

?>
```