# Handlersocket::auth #

Handlersocket::auth - Authenticate using a plain password.

# Description #

```
public bool HandlerSocket::auth( string $key, [, string $type ] )
```

# Parameters #

## key ##
| _string_ | password |
|:---------|:---------|

## type ##
| _string_ | authenticate type | this version not used |
|:---------|:------------------|:----------------------|

# Return Values #

Returns if the authenticate was successfully.

# Examples #

```
<?php

if (!$hs->auth('pass')) {
  die('Fault auth');
}

?>
```