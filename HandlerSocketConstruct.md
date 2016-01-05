# Handlersocket::construct #

Handlersocket::construct - Creates a new conection objects.

# Description #

```
HandlerSocket::__construct( string $host, string $port [, array $options ] )
```

# Parameters #

## host ##
| _string_ | host name |
|:---------|:----------|

## port ##
| _string_ | port number |
|:---------|:------------|

## options ##
| _array_ | options |
|:--------|:--------|

  * timeout
> connection timeout seconds.


# Return Values #

Returns the connection object.

# Errors/Exceptions #

Throws HandletSocketException if the connection is invalid.

# Examples #

```
<?php

try {
  $hs = new Handlersocker('localhost', 9999);
} catch (HandlerSocketException $e) {
  die($e->getMessage());
}

?>
```

## timeout setting ##

```
<?php

try {
  $hs = new Handlersocker('localhost', 9999, array('timeout' => 30));
} catch (HandlerSocketException $e) {
  die($e->getMessage());
}

?>
```