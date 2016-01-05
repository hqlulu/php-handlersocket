# Handlersocket::executeMulti #

Handlersocket::executeMulti - Execute

# Description #

```
public array HandlerSocket::executeMulti( array $args )
```

# Parameters #

## args ##
| _array_ | array of [Handlersocket::executeSingle](http://code.google.com/p/php-handlersocket/wiki/HandlerSocketExecuteSingle) parameter |
|:--------|:------------------------------------------------------------------------------------------------------------------------------|

# Return Values #

Returns the results.

# Examples #

```
<?php

$ret = $hs->executMulti(
    array(
        array(1, '>=', array('K1')),
        array(1, '>=', array('K1'), 3),
        array(1, '>=', array('K1'), 1, 0, null, null, array('F', '>', 0, 'F1')),
        array(1, '=', array('K1'), 1, 0, 'U', array('KEY1', 'VAL1'))
     )
);

var_dump($ret);

?>
```

The above example will output something similar to:

```
array(4) {
  array(1) {
    [0]=>
    array(2) {
      [0]=>
      string(2) "K1"
      [1]=>
      string(2) "V1"
    }
  }
  array(3) {
    [0]=>
    array(2) {
      [0]=>
      string(2) "K1"
      [1]=>
      string(2) "V1"
    }
    [1]=>
    array(2) {
      [0]=>
      string(2) "K2"
      [1]=>
      string(2) "V2"
    }
    [2]=>
    array(2) {
      [0]=>
      string(2) "K3"
      [1]=>
      string(2) "V3"
    }
  }
  array(1) {
    [0]=>
    array(2) {
      [0]=>
      string(2) "K1"
      [1]=>
      string(2) "V1"
    }
  }
  int(1)
}
```