# Handlersocket::openIndex #

Handlersocket::openIndex - Open a database index.

# Description #

```
public bool HandlerSocket::openIndex( long $id, string $db, string $table, string $index, string $field [, array $filter = array() ] )
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

## field ##
| _string_ | field lists |
|:---------|:------------|

Name of the field in the table.
Can specify multiple comma separated values.

## filter ##
| _string_ | filter lists |
|:---------|:-------------|

Name of the field in the table.
Can specify multiple comma separated values.

# Return Values #

Returns if the open was successfully.

# Examples #

```
<?php

try {
  $hs = new Handlersocker('localhost', 9999);
} catch (HandlerSocketException $e) {
  die($e->getMessage());
}

if (!$hs->openIndex(1, 'db', 'table', 'PRIMARY', 'k,v')) {
  die('Fault openIndex');
}

?>
```

## filter setting ##

```
<?php

$hs->openIndex(1, 'db', 'table', 'PRIMARY', 'k,v', 'f1,f2');

?>
```