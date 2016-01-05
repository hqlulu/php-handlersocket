# Handlersocket::executeSingle #

HandlerSocket::executeSingle - Execute

# Description #

```
public mixed HandlerSocket::executeSingle( long $id, string $operate, array $criteria [, long $limit = 1, long $offset = 0, string $update = null, array $values = array(), array $filters = array(), long $in_key = -1, array $in_values = array() ] )
```

# Parameters #

## id ##
| _long_ | id |
|:-------|:---|

## operate ##
| _string_ | comparison operator |
|:---------|:--------------------|

  * supported
> '=', '<', '<=', '>', '>='

## criteria ##
| _array_ | comparison values |
|:--------|:------------------|

## limit ##
| _long_ | limit value |
|:-------|:------------|

## offset ##
| _long_ | offset value |
|:-------|:-------------|

## update ##
| _string_ | update operator |
|:---------|:----------------|

  * supported
> U, +, -, D, U?, +?, -?, D?

## values ##
| _array_ | update values |
|:--------|:--------------|

## filters ##
| _array_ | filter values |
|:--------|:--------------|

Specifying an array of the form:

> | **key** | **value** |
|:--------|:----------|
> | 0       | filter operator (F or W) |
> | 1       | filter comarison operator |
> | 2       | filter position number |
> | 2       | filter comarison value |

## in\_key ##
| _long_ | index number of in field |
|:-------|:-------------------------|

## in\_values ##
| _array_ | in values |
|:--------|:----------|

# Return Values #

Returns the results.

# Examples #

```
<?php

$hs->openIndex(1, 'db', 'table', 'PRIMARY', 'k,v', 'f1,f2');

$ret = $hs->executSingle(1, '>=', array('K1'));
var_dump($ret);

?>
```

The above example will output something similar to:

```
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(2) "K1"
    [1]=>
    string(2) "V1"
  }
}
```

Is equivalent to following SQL statement processing.
```
SELECT k, v FROM table WHERE k >= 'K1' LIMIT 1
```

## limit and offset ##
```
<?php

$ret = $hs->executeSingle(1, '>=', array('K1'), 5);
//SELECT k, v FROM table WHERE k >= 'k1' LIMIT 5

$ret = $hs->executeSingle(1, '>=', array('K1'), 5, 3);
//SELECT k, v FROM table WHERE k >= 'k1' LIMIT 3, 5

?>
```

## filter ##
```
<?php

$ret = $hs->executeSingle(1, '>=', array('K1'), 1, 0, null, null, array('F', '>', 0, 'F1'));
//SELECT k, v FROM table WHERE k >= 'K1' AND f1 > 'F1' LIMIT 5

$ret = $hs->executeSingle(1, '>=', array('K1'), 10, 0, null, null, array(array('F', '>', 0, 'F1'), array('F', '<', 1, 'F10')));
//SELECT k, v FROM table WHERE k >= 'K1' AND f1 > 'F1' AND f2 <= 'F20' LIMIT 10

?>
```

## in ##
```
<?php

$ret = $hs->executeSingle(1, '>=', array('K1'), 3, 0, null, null, null, 0, array('K1', 'K3', 'K5'));
//SELECT k, v FROM table WHERE k IN ('K1', 'K3', 'K5') LIMIT 3

?>
```

## update ##
```
<?php

$ret = $hs->executeSingle(1, '=', array('K1'), 1, 0, 'U', array('KEY1', 'VAL1'));
//UPDATE table SET k = 'KEY1', v = 'VAL1' WHERE k = 'K1' LIMIT 1

?>
```

The above example will output something similar to:

```
int(1)
```

it returns the number of modified records.

```
<?php

$ret = $hs->executeSingle(1, '=', array('K1'), 1, 0, 'U?', array('KEY1', 'VAL1'));
//UPDATE table SET k = 'KEY1', v = 'VAL1' WHERE k = 'K1'

?>
```

The above example will output something similar to:

```
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(2) "K1"
    [1]=>
    string(2) "V1"
  }
}
```

If the '?' suffix is specified, it returns the contents of the records before modification.