# Handlersocket::ExecuteUpdate #

Handlersocket::ExecuteUpdate - Update

# Description #

```
public mixed HandlerSocket::executeUpdate( long $id, string $operate, array $criteria, array $values [, long $limit = 1, long $offset = 0, array $filters = array(), long $in_key  = -1, array $in_values = array() ] )
```

This method is the same as the 'U' argument to specify a fifth HandlerSocket::executeSingle.

# Parameters #
## id ##
| _long_ | id |
|:-------|:---|

## operate ##
| _string_ | comparison operator |
|:---------|:--------------------|

## criteria ##
| _array_ | comparison values |
|:--------|:------------------|

## values ##
| _array_ | update values |
|:--------|:--------------|

## limit ##
| _long_ | limit value |
|:-------|:------------|

## offset ##
| _long_ | offset value |
|:-------|:-------------|

## filters ##
| _array_ | filter values |
|:--------|:--------------|

## in\_key ##
| _long_ | index number of in field |
|:-------|:-------------------------|

## in\_values ##
| _array_ | in values |
|:--------|:----------|

Details of the argument is a reference [HandlerSocket::executeSingle](http://code.google.com/p/php-handlersocket/wiki/HandlerSocketExecuteSingle).

# Return Values #

Returns the results.

# Examples #

```
<?php

$ret = $hs->executeUpdate(1, '=', array('K1'), array('KEY1', 'VAL1'));
//UPDATE table SET k = 'KEY1', v = 'VAL1' WHERE k = 'K1'

var_dump($ret);

?>
```

The above example will output something similar to:

```
int(1)
```