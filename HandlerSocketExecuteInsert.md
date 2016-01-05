# Handlersocket::ExecuteInsert #

Handlersocket::ExecuteInsert - Insert

# Description #

```
public mixed HandlerSocket::executeInsert( long $id, array $field )
```

# Parameters #
## id ##
| _long_ | id |
|:-------|:---|

## field ##
| _array_ | array of insert fields |
|:--------|:-----------------------|

# Return Values #

Returns the results.

# Examples #

```
<?php

$hs->openIndex(1, 'db', 'table', 'PRIMARY', 'k,v');

$ret = $hs->executeInsert(1, array('K10', 'V10'));
//INSERT INTO table (k, v) VALUES ('K10', 'V10')

var_dump($ret);

?>
```

The above example will output something similar to:

```
int(1)
```